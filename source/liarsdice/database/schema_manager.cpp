#include <liarsdice/database/schema_manager.hpp>
#include <boost/regex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iomanip>
#include <sstream>

namespace liarsdice::database {

void SchemaManager::initialize_schema_table() {
    const char* create_sql = R"(
        CREATE TABLE IF NOT EXISTS schema_version (
            id TEXT PRIMARY KEY,
            version INTEGER UNIQUE NOT NULL,
            description TEXT NOT NULL,
            checksum TEXT NOT NULL,
            applied_at TIMESTAMP NOT NULL,
            execution_time_ms INTEGER
        );
        
        CREATE INDEX IF NOT EXISTS idx_schema_version ON schema_version(version);
    )";
    
    auto result = db_.execute(create_sql);
    if (!result) {
        BOOST_LOG_TRIVIAL(error) << "Failed to create schema_version table: " 
                                 << result.error().full_message();
    }
}

DatabaseResult<int> SchemaManager::load_migrations_from_directory(const boost::filesystem::path& dir) {
    if (!boost::filesystem::exists(dir) || !boost::filesystem::is_directory(dir)) {
        return DatabaseError(DatabaseErrorType::InvalidParameter, 
                           "Migration directory does not exist: " + dir.string());
    }
    
    // Pattern: V001__description.up.sql or V001__description.down.sql
    boost::regex pattern(R"(V(\d+)__(.+)\.(up|down)\.sql)");
    std::map<int, std::pair<boost::filesystem::path, boost::filesystem::path>> migration_files;
    
    try {
        for (const auto& entry : boost::filesystem::directory_iterator(dir)) {
            if (!boost::filesystem::is_regular_file(entry)) continue;
            
            std::string filename = entry.path().filename().string();
            boost::smatch match;
            
            if (boost::regex_match(filename, match, pattern)) {
                int version = std::stoi(match[1]);
                std::string type = match[3];
                
                if (type == "up") {
                    migration_files[version].first = entry.path();
                } else if (type == "down") {
                    migration_files[version].second = entry.path();
                }
            }
        }
        
        // Create migration objects
        int loaded_count = 0;
        for (const auto& [version, paths] : migration_files) {
            if (!paths.first.empty()) {
                auto migration = std::make_unique<FileMigration>(version, paths.first, paths.second);
                add_migration(std::move(migration));
                loaded_count++;
            }
        }
        
        BOOST_LOG_TRIVIAL(info) << "Loaded " << loaded_count << " migrations from " << dir;
        return loaded_count;
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Failed to load migrations: " + std::string(e.what()));
    }
}

DatabaseResult<int> SchemaManager::get_current_version() {
    auto stmt = db_.prepare("SELECT MAX(version) FROM schema_version");
    if (!stmt) {
        return DatabaseError(stmt.error());
    }
    
    int version = 0;
    db_.execute_prepared(stmt.value(), [&version](const PreparedStatement& row) {
        auto col = row.get_column(0);
        // Check if the value is NULL (monostate)
        if (!std::holds_alternative<std::monostate>(col)) {
            version = static_cast<int>(std::get<int64_t>(col));
        }
    });
    
    return version;
}

DatabaseResult<std::vector<SchemaVersion>> SchemaManager::get_applied_migrations() {
    auto stmt = db_.prepare(R"(
        SELECT id, version, description, checksum, applied_at
        FROM schema_version
        ORDER BY version
    )");
    
    if (!stmt) {
        return DatabaseError(stmt.error());
    }
    
    std::vector<SchemaVersion> versions;
    auto result = db_.execute_prepared(stmt.value(), 
        [&versions](const PreparedStatement& row) {
            SchemaVersion ver;
            ver.id = std::get<std::string>(row.get_column(0));
            ver.version = static_cast<int>(std::get<int64_t>(row.get_column(1)));
            ver.description = std::get<std::string>(row.get_column(2));
            ver.checksum = std::get<std::string>(row.get_column(3));
            
            // Parse timestamp
            std::string timestamp_str = std::get<std::string>(row.get_column(4));
            std::istringstream iss(timestamp_str);
            std::tm tm = {};
            iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            ver.applied_at = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            
            ver.is_applied = true;
            versions.push_back(ver);
        });
    
    if (!result) {
        return DatabaseError(result.error());
    }
    
    return versions;
}

DatabaseResult<bool> SchemaManager::is_version_applied(int version) {
    auto stmt = db_.prepare("SELECT 1 FROM schema_version WHERE version = ?");
    if (!stmt) {
        return DatabaseError(stmt.error());
    }
    
    stmt.value()->bind(1, version);
    
    bool exists = false;
    db_.execute_prepared(stmt.value(), [&exists](const PreparedStatement&) {
        exists = true;
    });
    
    return exists;
}

DatabaseResult<void> SchemaManager::migrate_to(int target_version) {
    boost::lock_guard<boost::mutex> lock(mutex_);
    
    auto current_result = get_current_version();
    if (!current_result) {
        return DatabaseError(current_result.error());
    }
    
    int current_version = current_result.value();
    
    // Determine target
    if (target_version == -1) {
        if (!migrations_.empty()) {
            target_version = migrations_.back()->get_version().version;
        } else {
            return DatabaseResult<void>(); // Nothing to do
        }
    }
    
    if (target_version < current_version) {
        return rollback_to(target_version);
    }
    
    // Apply migrations in order
    return db_.with_transaction([this, current_version, target_version](DatabaseManager& mgr) -> DatabaseResult<void> {
        for (const auto& migration : migrations_) {
            int version = migration->get_version().version;
            
            if (version > current_version && version <= target_version) {
                auto is_applied = is_version_applied(version);
                if (!is_applied) {
                    return DatabaseError(is_applied.error());
                }
                
                if (!is_applied.value()) {
                    BOOST_LOG_TRIVIAL(info) << "Applying migration " << version 
                                           << ": " << migration->get_version().description;
                    
                    auto start = std::chrono::steady_clock::now();
                    auto result = execute_migration(migration.get());
                    if (!result) {
                        return result;
                    }
                    
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start);
                    
                    BOOST_LOG_TRIVIAL(info) << "Migration " << version 
                                           << " applied in " << duration.count() << "ms";
                }
            }
        }
        
        return DatabaseResult<void>();
    });
}

DatabaseResult<void> SchemaManager::rollback_to(int target_version) {
    boost::lock_guard<boost::mutex> lock(mutex_);
    
    auto current_result = get_current_version();
    if (!current_result) {
        return DatabaseError(current_result.error());
    }
    
    int current_version = current_result.value();
    
    if (target_version >= current_version) {
        return DatabaseResult<void>(); // Nothing to rollback
    }
    
    // Find migrations to rollback (in reverse order)
    std::vector<Migration*> to_rollback;
    for (auto it = migrations_.rbegin(); it != migrations_.rend(); ++it) {
        int version = (*it)->get_version().version;
        if (version > target_version && version <= current_version) {
            if (!(*it)->is_reversible()) {
                return DatabaseError(DatabaseErrorType::InvalidParameter,
                    "Migration " + std::to_string(version) + " is not reversible");
            }
            to_rollback.push_back(it->get());
        }
    }
    
    // Execute rollbacks in transaction
    return db_.with_transaction([this, &to_rollback](DatabaseManager& mgr) -> DatabaseResult<void> {
        for (auto* migration : to_rollback) {
            int version = migration->get_version().version;
            
            BOOST_LOG_TRIVIAL(info) << "Rolling back migration " << version;
            
            auto result = execute_rollback(migration);
            if (!result) {
                return result;
            }
            
            BOOST_LOG_TRIVIAL(info) << "Migration " << version << " rolled back";
        }
        
        return DatabaseResult<void>();
    });
}

DatabaseResult<std::vector<Migration*>> SchemaManager::get_pending_migrations() {
    auto current_result = get_current_version();
    if (!current_result) {
        return DatabaseError(current_result.error());
    }
    
    int current_version = current_result.value();
    std::vector<Migration*> pending;
    
    for (const auto& migration : migrations_) {
        if (migration->get_version().version > current_version) {
            pending.push_back(migration.get());
        }
    }
    
    return pending;
}

DatabaseResult<void> SchemaManager::validate_migrations() {
    // Check for gaps in version numbers
    int expected_version = 1;
    for (const auto& migration : migrations_) {
        int version = migration->get_version().version;
        if (version != expected_version) {
            return DatabaseError(DatabaseErrorType::InvalidParameter,
                "Migration version gap detected: expected " + 
                std::to_string(expected_version) + ", found " + std::to_string(version));
        }
        expected_version++;
    }
    
    // Verify applied migrations match their checksums
    auto applied_result = get_applied_migrations();
    if (!applied_result) {
        return DatabaseError(applied_result.error());
    }
    
    for (const auto& applied : applied_result.value()) {
        // Find corresponding migration
        auto it = std::find_if(migrations_.begin(), migrations_.end(),
            [&applied](const auto& m) {
                return m->get_version().version == applied.version;
            });
        
        if (it != migrations_.end()) {
            if ((*it)->get_version().checksum != applied.checksum) {
                return DatabaseError(DatabaseErrorType::InvalidParameter,
                    "Checksum mismatch for migration " + std::to_string(applied.version));
            }
        }
    }
    
    return DatabaseResult<void>();
}

DatabaseResult<void> SchemaManager::clean_migration_history() {
    // Remove migration records that don't have corresponding migration files
    auto applied_result = get_applied_migrations();
    if (!applied_result) {
        return DatabaseError(applied_result.error());
    }
    
    return db_.with_transaction([this, &applied_result](DatabaseManager& mgr) -> DatabaseResult<void> {
        for (const auto& applied : applied_result.value()) {
            auto it = std::find_if(migrations_.begin(), migrations_.end(),
                [&applied](const auto& m) {
                    return m->get_version().version == applied.version;
                });
            
            if (it == migrations_.end()) {
                auto result = remove_migration_record(applied.version);
                if (!result) {
                    return result;
                }
                
                BOOST_LOG_TRIVIAL(warning) << "Removed orphaned migration record: " 
                                          << applied.version;
            }
        }
        
        return DatabaseResult<void>();
    });
}

DatabaseResult<void> SchemaManager::baseline(int version, const std::string& description) {
    SchemaVersion baseline_version(version, description);
    baseline_version.applied_at = std::chrono::system_clock::now();
    
    return record_migration(baseline_version);
}

DatabaseResult<void> SchemaManager::record_migration(const SchemaVersion& version) {
    auto stmt = db_.prepare(R"(
        INSERT INTO schema_version (id, version, description, checksum, applied_at)
        VALUES (?, ?, ?, ?, datetime('now'))
    )");
    
    if (!stmt) {
        return DatabaseError(stmt.error());
    }
    
    stmt.value()->bind(1, version.id);
    stmt.value()->bind(2, version.version);
    stmt.value()->bind(3, version.description);
    stmt.value()->bind(4, version.checksum);
    
    auto result = db_.execute_prepared(stmt.value());
    if (!result) {
        return DatabaseError(result.error());
    }
    return DatabaseResult<void>();
}

DatabaseResult<void> SchemaManager::remove_migration_record(int version) {
    auto stmt = db_.prepare("DELETE FROM schema_version WHERE version = ?");
    if (!stmt) {
        return DatabaseError(stmt.error());
    }
    
    stmt.value()->bind(1, version);
    
    auto result = db_.execute_prepared(stmt.value());
    if (!result) {
        return DatabaseError(result.error());
    }
    return DatabaseResult<void>();
}

DatabaseResult<void> SchemaManager::execute_migration(Migration* migration) {
    try {
        migration->apply(db_);
        return record_migration(migration->get_version());
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::QueryFailed, e.what());
    }
}

DatabaseResult<void> SchemaManager::execute_rollback(Migration* migration) {
    try {
        migration->rollback(db_);
        return remove_migration_record(migration->get_version().version);
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::QueryFailed, e.what());
    }
}

std::pair<int, std::string> SchemaManager::parse_migration_filename(const std::string& filename) {
    boost::regex pattern(R"(V(\d+)__(.+)\.(up|down)\.sql)");
    boost::smatch match;
    
    if (boost::regex_match(filename, match, pattern)) {
        return {std::stoi(match[1]), match[2]};
    }
    
    throw std::runtime_error("Invalid migration filename format: " + filename);
}

} // namespace liarsdice::database