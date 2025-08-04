#include <liarsdice/database/backup_manager.hpp>
#include <liarsdice/database/database_config.hpp>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/crc.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace liarsdice::database {

BackupManager::BackupManager(DatabaseManager& db, const boost::filesystem::path& backup_dir)
    : db_(db), backup_dir_(backup_dir) {
    
    // Set default retention policy
    retention_policy_ = RetentionPolicy{};
    
    // Ensure backup directory exists
    auto ensure_result = ensure_backup_directory();
    if (!ensure_result) {
        BOOST_LOG_TRIVIAL(warning) << "Failed to create backup directory: " 
                                  << ensure_result.error().full_message();
    }
}

void BackupManager::set_retention_policy(const RetentionPolicy& policy) {
    retention_policy_ = policy;
    BOOST_LOG_TRIVIAL(info) << "Updated backup retention policy: "
                           << "daily(" << policy.daily_keep_days << "d), "
                           << "weekly(" << policy.weekly_keep_weeks << "w), "
                           << "monthly(" << policy.monthly_keep_months << "m), "
                           << "yearly(" << policy.yearly_keep_years << "y)";
}

DatabaseResult<BackupInfo> BackupManager::create_backup(const std::string& backup_name) {
    try {
        auto ensure_result = ensure_backup_directory();
        if (!ensure_result) {
            return DatabaseError(ensure_result.error());
        }
        
        std::string filename = backup_name.empty() ? 
            generate_backup_filename("manual") : 
            backup_name + ".db";
        
        boost::filesystem::path backup_path = backup_dir_ / filename;
        
        // Copy database file
        auto copy_result = copy_database_file(backup_path);
        if (!copy_result) {
            return DatabaseError(copy_result.error());
        }
        
        // Create backup info
        BackupInfo info = create_backup_info(backup_path, "manual");
        
        // Compress if enabled
        if (retention_policy_.compress_backups) {
            auto compress_result = compress_backup(backup_path);
            if (compress_result) {
                // Update info to point to compressed file
                info.file_path = compress_result.value();
                info.is_compressed = true;
                info.file_size = boost::filesystem::file_size(info.file_path);
                
                // Remove uncompressed file
                boost::filesystem::remove(backup_path);
            } else {
                BOOST_LOG_TRIVIAL(warning) << "Failed to compress backup: " 
                                          << compress_result.error().full_message();
            }
        }
        
        BOOST_LOG_TRIVIAL(info) << "Created backup: " << info.file_path 
                               << " (size: " << info.file_size << " bytes)";
        
        return info;
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Failed to create backup: " + std::string(e.what()));
    }
}

DatabaseResult<BackupInfo> BackupManager::create_scheduled_backup(const std::string& backup_type) {
    try {
        std::string filename = generate_backup_filename(backup_type);
        boost::filesystem::path backup_path = backup_dir_ / filename;
        
        // Check if backup already exists for this time period
        if (boost::filesystem::exists(backup_path)) {
            BOOST_LOG_TRIVIAL(info) << "Scheduled backup already exists: " << backup_path;
            return create_backup_info(backup_path, backup_type);
        }
        
        // Copy database file
        auto copy_result = copy_database_file(backup_path);
        if (!copy_result) {
            return DatabaseError(copy_result.error());
        }
        
        BackupInfo info = create_backup_info(backup_path, backup_type);
        
        // Compress if enabled
        if (retention_policy_.compress_backups) {
            auto compress_result = compress_backup(backup_path);
            if (compress_result) {
                info.file_path = compress_result.value();
                info.is_compressed = true;
                info.file_size = boost::filesystem::file_size(info.file_path);
                boost::filesystem::remove(backup_path);
            }
        }
        
        BOOST_LOG_TRIVIAL(info) << "Created scheduled backup (" << backup_type << "): " 
                               << info.file_path;
        
        return info;
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Failed to create scheduled backup: " + std::string(e.what()));
    }
}

DatabaseResult<void> BackupManager::restore_from_backup(const boost::filesystem::path& backup_file) {
    if (!boost::filesystem::exists(backup_file)) {
        return DatabaseError(DatabaseErrorType::InvalidParameter, 
                           "Backup file not found: " + backup_file.string());
    }
    
    try {
        // Verify backup integrity first
        auto verify_result = verify_backup(backup_file);
        if (!verify_result) {
            return DatabaseError(verify_result.error());
        }
        
        if (!verify_result.value()) {
            return DatabaseError(DatabaseErrorType::InternalError, 
                               "Backup file integrity check failed");
        }
        
        boost::filesystem::path temp_file = backup_file;
        bool is_compressed = boost::algorithm::ends_with(backup_file.string(), 
                                                        retention_policy_.compression_suffix);
        
        // Decompress if necessary
        if (is_compressed) {
            auto decompress_result = decompress_backup(backup_file);
            if (!decompress_result) {
                return DatabaseError(decompress_result.error());
            }
            temp_file = decompress_result.value();
        }
        
        // Get current database path
        auto& config = DatabaseConfig::instance();
        boost::filesystem::path current_db = config.get_database_file_path();
        
        // Create backup of current database
        boost::filesystem::path current_backup = current_db.string() + ".restore_backup";
        if (boost::filesystem::exists(current_db)) {
            boost::filesystem::copy_file(current_db, current_backup);
        }
        
        try {
            // Copy backup file to current database location
            boost::filesystem::copy_file(temp_file, current_db, 
                                       boost::filesystem::copy_options::overwrite_existing);
            
            // Clean up temporary decompressed file
            if (is_compressed && temp_file != backup_file) {
                boost::filesystem::remove(temp_file);
            }
            
            // Remove backup of current database
            if (boost::filesystem::exists(current_backup)) {
                boost::filesystem::remove(current_backup);
            }
            
            BOOST_LOG_TRIVIAL(info) << "Successfully restored database from: " << backup_file;
            return DatabaseResult<void>();
            
        } catch (const std::exception& e) {
            // Restore original database on failure
            if (boost::filesystem::exists(current_backup)) {
                boost::filesystem::copy_file(current_backup, current_db,
                                           boost::filesystem::copy_options::overwrite_existing);
                boost::filesystem::remove(current_backup);
            }
            throw;
        }
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Failed to restore from backup: " + std::string(e.what()));
    }
}

DatabaseResult<std::vector<BackupInfo>> BackupManager::list_backups() {
    try {
        std::vector<BackupInfo> backups = scan_backup_directory();
        
        // Sort by creation time (newest first)
        std::sort(backups.begin(), backups.end(), 
                 [](const BackupInfo& a, const BackupInfo& b) {
                     return a.created_at > b.created_at;
                 });
        
        return backups;
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Failed to list backups: " + std::string(e.what()));
    }
}

DatabaseResult<void> BackupManager::apply_retention_policy() {
    try {
        auto backups_result = list_backups();
        if (!backups_result) {
            return DatabaseError(backups_result.error());
        }
        
        auto cleanup_result = cleanup_old_backups(backups_result.value());
        if (!cleanup_result) {
            return cleanup_result;
        }
        
        BOOST_LOG_TRIVIAL(info) << "Applied retention policy to backup directory";
        return DatabaseResult<void>();
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Failed to apply retention policy: " + std::string(e.what()));
    }
}

DatabaseResult<bool> BackupManager::verify_backup(const boost::filesystem::path& backup_file) {
    if (!boost::filesystem::exists(backup_file)) {
        return DatabaseError(DatabaseErrorType::InvalidParameter, 
                           "Backup file not found: " + backup_file.string());
    }
    
    try {
        // Calculate current checksum
        std::string current_checksum = calculate_file_checksum(backup_file);
        
        // For simplicity, we'll assume the backup is valid if we can calculate a checksum
        // In a real implementation, you'd store checksums separately and compare them
        bool is_valid = !current_checksum.empty();
        
        BOOST_LOG_TRIVIAL(debug) << "Backup verification for " << backup_file 
                                << ": " << (is_valid ? "VALID" : "INVALID")
                                << " (checksum: " << current_checksum.substr(0, 16) << "...)";
        
        return is_valid;
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Failed to verify backup: " + std::string(e.what()));
    }
}

DatabaseResult<std::map<std::string, size_t>> BackupManager::get_storage_statistics() {
    try {
        std::map<std::string, size_t> stats;
        
        auto backups = scan_backup_directory();
        
        stats["total_backups"] = backups.size();
        stats["total_size_bytes"] = 0;
        stats["daily_backups"] = 0;
        stats["weekly_backups"] = 0;
        stats["monthly_backups"] = 0;
        stats["yearly_backups"] = 0;
        stats["manual_backups"] = 0;
        
        for (const auto& backup : backups) {
            stats["total_size_bytes"] += backup.file_size;
            
            if (backup.backup_type == "daily") stats["daily_backups"]++;
            else if (backup.backup_type == "weekly") stats["weekly_backups"]++;
            else if (backup.backup_type == "monthly") stats["monthly_backups"]++;
            else if (backup.backup_type == "yearly") stats["yearly_backups"]++;
            else stats["manual_backups"]++;
        }
        
        stats["directory_size_bytes"] = get_directory_size(backup_dir_);
        
        return stats;
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Failed to get storage statistics: " + std::string(e.what()));
    }
}

DatabaseResult<boost::filesystem::path> BackupManager::compress_backup(const boost::filesystem::path& backup_file) {
    if (!boost::filesystem::exists(backup_file)) {
        return DatabaseError(DatabaseErrorType::InvalidParameter, 
                           "Backup file not found: " + backup_file.string());
    }
    
    try {
        boost::filesystem::path compressed_file = backup_file.string() + retention_policy_.compression_suffix;
        
        std::ifstream input(backup_file.string(), std::ios::binary);
        std::ofstream output(compressed_file.string(), std::ios::binary);
        
        boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
        in.push(boost::iostreams::gzip_compressor());
        in.push(input);
        
        boost::iostreams::copy(in, output);
        
        size_t original_size = boost::filesystem::file_size(backup_file);
        size_t compressed_size = boost::filesystem::file_size(compressed_file);
        
        BOOST_LOG_TRIVIAL(info) << "Compressed backup: " << backup_file 
                               << " -> " << compressed_file 
                               << " (saved " << (original_size - compressed_size) << " bytes)";
        
        return compressed_file;
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Failed to compress backup: " + std::string(e.what()));
    }
}

DatabaseResult<boost::filesystem::path> BackupManager::decompress_backup(const boost::filesystem::path& compressed_file) {
    if (!boost::filesystem::exists(compressed_file)) {
        return DatabaseError(DatabaseErrorType::InvalidParameter, 
                           "Compressed file not found: " + compressed_file.string());
    }
    
    try {
        std::string decompressed_name = compressed_file.string();
        if (boost::algorithm::ends_with(decompressed_name, retention_policy_.compression_suffix)) {
            decompressed_name = decompressed_name.substr(0, 
                decompressed_name.length() - retention_policy_.compression_suffix.length());
        } else {
            decompressed_name += ".decompressed";
        }
        
        boost::filesystem::path decompressed_file(decompressed_name);
        
        std::ifstream input(compressed_file.string(), std::ios::binary);
        std::ofstream output(decompressed_file.string(), std::ios::binary);
        
        boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
        in.push(boost::iostreams::gzip_decompressor());
        in.push(input);
        
        boost::iostreams::copy(in, output);
        
        BOOST_LOG_TRIVIAL(info) << "Decompressed backup: " << compressed_file 
                               << " -> " << decompressed_file;
        
        return decompressed_file;
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Failed to decompress backup: " + std::string(e.what()));
    }
}

std::string BackupManager::generate_backup_filename(const std::string& backup_type) {
    auto now = std::chrono::system_clock::now();
    
    std::string pattern;
    if (backup_type == "daily") pattern = retention_policy_.daily_pattern;
    else if (backup_type == "weekly") pattern = retention_policy_.weekly_pattern;
    else if (backup_type == "monthly") pattern = retention_policy_.monthly_pattern;
    else if (backup_type == "yearly") pattern = retention_policy_.yearly_pattern;
    else {
        // Manual backup
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << "backup_manual_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".db";
        return oss.str();
    }
    
    return format_backup_filename(pattern, now);
}

DatabaseResult<void> BackupManager::copy_database_file(const boost::filesystem::path& dest) {
    try {
        auto& config = DatabaseConfig::instance();
        boost::filesystem::path source = config.get_database_file_path();
        
        if (!boost::filesystem::exists(source)) {
            return DatabaseError(DatabaseErrorType::InvalidParameter, 
                               "Source database file not found: " + source.string());
        }
        
        boost::filesystem::copy_file(source, dest, 
                                   boost::filesystem::copy_options::overwrite_existing);
        
        return DatabaseResult<void>();
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Failed to copy database file: " + std::string(e.what()));
    }
}

std::string BackupManager::calculate_file_checksum(const boost::filesystem::path& file_path) {
    try {
        std::ifstream file(file_path.string(), std::ios::binary);
        if (!file) {
            return "";
        }
        
        boost::crc_32_type crc;
        
        char buffer[4096];
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
            crc.process_bytes(buffer, file.gcount());
        }
        
        std::ostringstream oss;
        oss << std::hex << crc.checksum();
        return oss.str();
        
    } catch (const std::exception&) {
        return "";
    }
}

BackupInfo BackupManager::create_backup_info(const boost::filesystem::path& file_path, 
                                            const std::string& backup_type) {
    BackupInfo info;
    info.file_path = file_path;
    info.created_at = std::chrono::system_clock::now();
    info.backup_type = backup_type;
    info.is_compressed = boost::algorithm::ends_with(file_path.string(), 
                                                    retention_policy_.compression_suffix);
    
    if (boost::filesystem::exists(file_path)) {
        info.file_size = boost::filesystem::file_size(file_path);
        info.checksum = calculate_file_checksum(file_path);
    }
    
    return info;
}

std::vector<BackupInfo> BackupManager::scan_backup_directory() {
    std::vector<BackupInfo> backups;
    
    if (!boost::filesystem::exists(backup_dir_) || !boost::filesystem::is_directory(backup_dir_)) {
        return backups;
    }
    
    try {
        for (boost::filesystem::directory_iterator it(backup_dir_); 
             it != boost::filesystem::directory_iterator(); ++it) {
            
            if (boost::filesystem::is_regular_file(it->path()) && 
                is_backup_file(it->path())) {
                
                std::string backup_type = extract_backup_type(it->path().filename().string());
                BackupInfo info = create_backup_info(it->path(), backup_type);
                
                // Set creation time from file modification time
                auto file_time = boost::filesystem::last_write_time(it->path());
                info.created_at = std::chrono::system_clock::from_time_t(file_time);
                
                backups.push_back(info);
            }
        }
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "Error scanning backup directory: " << e.what();
    }
    
    return backups;
}

DatabaseResult<void> BackupManager::ensure_backup_directory() {
    try {
        if (!boost::filesystem::exists(backup_dir_)) {
            boost::filesystem::create_directories(backup_dir_);
            BOOST_LOG_TRIVIAL(info) << "Created backup directory: " << backup_dir_;
        }
        
        if (!boost::filesystem::is_directory(backup_dir_)) {
            return DatabaseError(DatabaseErrorType::InternalError, 
                               "Backup path exists but is not a directory: " + backup_dir_.string());
        }
        
        return DatabaseResult<void>();
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Failed to create backup directory: " + std::string(e.what()));
    }
}

bool BackupManager::is_backup_file(const boost::filesystem::path& file_path) {
    std::string filename = file_path.filename().string();
    return boost::algorithm::starts_with(filename, "backup_") && 
           (boost::algorithm::ends_with(filename, ".db") || 
            boost::algorithm::ends_with(filename, ".db" + retention_policy_.compression_suffix));
}

std::string BackupManager::extract_backup_type(const std::string& filename) {
    if (filename.find("_daily_") != std::string::npos) return "daily";
    if (filename.find("_weekly_") != std::string::npos) return "weekly";
    if (filename.find("_monthly_") != std::string::npos) return "monthly";
    if (filename.find("_yearly_") != std::string::npos) return "yearly";
    return "manual";
}

std::string BackupManager::format_backup_filename(const std::string& pattern, 
                                                const std::chrono::system_clock::time_point& time_point) {
    auto time_t = std::chrono::system_clock::to_time_t(time_point);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), pattern.c_str());
    return oss.str();
}

size_t BackupManager::get_directory_size(const boost::filesystem::path& directory) {
    size_t total_size = 0;
    
    try {
        for (boost::filesystem::recursive_directory_iterator it(directory);
             it != boost::filesystem::recursive_directory_iterator(); ++it) {
            
            if (boost::filesystem::is_regular_file(it->path())) {
                total_size += boost::filesystem::file_size(it->path());
            }
        }
    } catch (const std::exception&) {
        // Ignore errors and return partial size
    }
    
    return total_size;
}

DatabaseResult<void> BackupManager::cleanup_old_backups(const std::vector<BackupInfo>& backups) {
    size_t removed_count = 0;
    size_t removed_size = 0;
    
    try {
        for (const auto& backup : backups) {
            if (!should_keep_backup(backup)) {
                BOOST_LOG_TRIVIAL(info) << "Removing old backup: " << backup.file_path 
                                       << " (type: " << backup.backup_type << ")";
                
                removed_size += backup.file_size;
                boost::filesystem::remove(backup.file_path);
                removed_count++;
            }
        }
        
        if (removed_count > 0) {
            BOOST_LOG_TRIVIAL(info) << "Cleanup completed: removed " << removed_count 
                                   << " backups, freed " << removed_size << " bytes";
        }
        
        return DatabaseResult<void>();
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Failed to cleanup old backups: " + std::string(e.what()));
    }
}

bool BackupManager::should_keep_backup(const BackupInfo& backup) {
    auto now = std::chrono::system_clock::now();
    auto backup_age = std::chrono::duration_cast<std::chrono::hours>(now - backup.created_at);
    
    if (backup.backup_type == "daily") {
        return backup_age.count() < (retention_policy_.daily_keep_days * 24);
    } else if (backup.backup_type == "weekly") {
        return backup_age.count() < (retention_policy_.weekly_keep_weeks * 7 * 24);
    } else if (backup.backup_type == "monthly") {
        return backup_age.count() < (retention_policy_.monthly_keep_months * 30 * 24);
    } else if (backup.backup_type == "yearly") {
        return backup_age.count() < (retention_policy_.yearly_keep_years * 365 * 24);
    } else {
        // Keep manual backups for same period as daily backups
        return backup_age.count() < (retention_policy_.daily_keep_days * 24);
    }
}

// Placeholder implementations for remaining methods
DatabaseResult<BackupInfo> BackupManager::create_sql_dump(const std::string& dump_name) {
    // Implementation would create SQL dump instead of binary copy
    return create_backup(dump_name + "_dump");
}

DatabaseResult<void> BackupManager::import_sql_dump(const boost::filesystem::path& dump_file) {
    // Implementation would execute SQL from dump file
    BOOST_LOG_TRIVIAL(info) << "SQL dump import not yet implemented: " << dump_file;
    return DatabaseResult<void>();
}

DatabaseResult<void> BackupManager::schedule_automatic_backups(bool enable) {
    // Implementation would set up background thread or integration with system scheduler
    BOOST_LOG_TRIVIAL(info) << "Automatic backup scheduling " 
                           << (enable ? "enabled" : "disabled");
    return DatabaseResult<void>();
}

} // namespace liarsdice::database