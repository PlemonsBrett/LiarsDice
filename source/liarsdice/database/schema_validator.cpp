#include <liarsdice/database/schema_validator.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/log/trivial.hpp>
#include <algorithm>
#include <sstream>

namespace liarsdice::database {

void SchemaValidator::initialize_constraints() {
    // Common naming convention constraints
    constraints_.clear();
    
    // Table naming: lowercase with underscores
    SchemaConstraint table_naming("*", "*", ConstraintType::Check, "table_naming");
    table_naming.validator = [this](const std::string& name) {
        return validate_table_name(name);
    };
    constraints_.push_back(table_naming);
    
    // Column naming: lowercase with underscores
    SchemaConstraint column_naming("*", "*", ConstraintType::Check, "column_naming");
    column_naming.validator = [this](const std::string& name) {
        return validate_column_name(name);
    };
    constraints_.push_back(column_naming);
    
    // Primary key constraints for main tables
    constraints_.push_back(SchemaConstraint("players", "id", ConstraintType::PrimaryKey, "pk_players"));
    constraints_.push_back(SchemaConstraint("game_history", "id", ConstraintType::PrimaryKey, "pk_game_history"));
    constraints_.push_back(SchemaConstraint("achievements", "id", ConstraintType::PrimaryKey, "pk_achievements"));
    
    // Foreign key constraints
    constraints_.push_back(SchemaConstraint("game_history", "winner_id", ConstraintType::ForeignKey, 
        "fk_game_winner", "REFERENCES players(id)"));
    constraints_.push_back(SchemaConstraint("game_participants", "player_id", ConstraintType::ForeignKey, 
        "fk_participant_player", "REFERENCES players(id)"));
    constraints_.push_back(SchemaConstraint("game_participants", "game_id", ConstraintType::ForeignKey, 
        "fk_participant_game", "REFERENCES game_history(id)"));
        
    // Unique constraints
    constraints_.push_back(SchemaConstraint("players", "username", ConstraintType::Unique, "uk_players_username"));
    constraints_.push_back(SchemaConstraint("achievements", "code", ConstraintType::Unique, "uk_achievements_code"));
    
    // Not null constraints for critical fields
    constraints_.push_back(SchemaConstraint("players", "username", ConstraintType::NotNull));
    constraints_.push_back(SchemaConstraint("players", "display_name", ConstraintType::NotNull));
    constraints_.push_back(SchemaConstraint("game_history", "started_at", ConstraintType::NotNull));
    constraints_.push_back(SchemaConstraint("achievements", "code", ConstraintType::NotNull));
    constraints_.push_back(SchemaConstraint("achievements", "name", ConstraintType::NotNull));
}

DatabaseResult<ValidationReport> SchemaValidator::validate_schema() {
    ValidationReport report;
    
    try {
        // Get all tables
        auto tables = get_table_names();
        if (tables.empty()) {
            report.add_warning("No user tables found in database");
            return report;
        }
        
        BOOST_LOG_TRIVIAL(info) << "Validating schema for " << tables.size() << " tables";
        
        // Validate each table
        for (const auto& table : tables) {
            auto table_result = validate_table(table);
            if (!table_result) {
                report.add_error("Failed to validate table '" + table + "': " + 
                               table_result.error().full_message());
                continue;
            }
            
            const auto& table_report = table_result.value();
            if (!table_report.is_valid) {
                report.is_valid = false;
                for (const auto& error : table_report.errors) {
                    report.errors.push_back(error);
                }
            }
            
            for (const auto& warning : table_report.warnings) {
                report.warnings.push_back(warning);
            }
        }
        
        // Validate naming conventions
        auto naming_result = validate_naming_conventions();
        if (naming_result && !naming_result.value().is_valid) {
            report.is_valid = false;
            for (const auto& error : naming_result.value().errors) {
                report.errors.push_back(error);
            }
        }
        
        // Validate foreign key relationships
        auto fk_result = validate_foreign_keys();
        if (fk_result && !fk_result.value().is_valid) {
            report.is_valid = false;
            for (const auto& error : fk_result.value().errors) {
                report.errors.push_back(error);
            }
        }
        
        // Validate indexes
        auto index_result = validate_indexes();
        if (index_result) {
            for (const auto& warning : index_result.value().warnings) {
                report.warnings.push_back(warning);
            }
        }
        
        BOOST_LOG_TRIVIAL(info) << "Schema validation completed: " 
                              << (report.is_valid ? "VALID" : "INVALID") 
                              << " (" << report.errors.size() << " errors, " 
                              << report.warnings.size() << " warnings)";
        
        return report;
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Schema validation failed: " + std::string(e.what()));
    }
}

DatabaseResult<ValidationReport> SchemaValidator::validate_table(const std::string& table_name) {
    ValidationReport report;
    
    try {
        auto schema = get_table_schema(table_name);
        if (schema.empty()) {
            report.add_table_issue(table_name, "Table schema not found");
            return report;
        }
        
        // Check table-specific constraints
        for (const auto& constraint : constraints_) {
            if (constraint.table_name != "*" && constraint.table_name != table_name) {
                continue;
            }
            
            switch (constraint.type) {
                case ConstraintType::PrimaryKey:
                    if (!check_constraint_exists(table_name, "PRIMARY KEY")) {
                        report.add_table_issue(table_name, "Missing primary key constraint");
                    }
                    break;
                    
                case ConstraintType::NotNull:
                    if (schema.count("sql") && schema["sql"].find(constraint.column_name + " ") != std::string::npos) {
                        if (schema["sql"].find("NOT NULL") == std::string::npos) {
                            report.add_table_issue(table_name, "Column '" + constraint.column_name + 
                                                 "' should have NOT NULL constraint");
                        }
                    }
                    break;
                    
                case ConstraintType::Unique:
                    if (schema.count("sql") && schema["sql"].find(constraint.column_name + " ") != std::string::npos) {
                        if (schema["sql"].find("UNIQUE") == std::string::npos) {
                            report.add_table_issue(table_name, "Column '" + constraint.column_name + 
                                                 "' should have UNIQUE constraint");
                        }
                    }
                    break;
                    
                default:
                    break;
            }
        }
        
        return report;
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Table validation failed: " + std::string(e.what()));
    }
}

DatabaseResult<ValidationReport> SchemaValidator::validate_naming_conventions() {
    ValidationReport report;
    
    try {
        auto tables = get_table_names();
        
        // Check table naming conventions using boost::algorithm
        for (const auto& table : tables) {
            std::string trimmed = boost::algorithm::trim_copy(table);
            
            if (!validate_table_name(trimmed)) {
                report.add_error("Table name '" + table + "' violates naming convention (should be lowercase_with_underscores)");
            }
            
            // Check for reserved words
            std::string upper_table = boost::algorithm::to_upper_copy(trimmed);
            std::vector<std::string> reserved = {"SELECT", "FROM", "WHERE", "INSERT", "UPDATE", "DELETE", "TABLE", "INDEX"};
            
            if (std::find(reserved.begin(), reserved.end(), upper_table) != reserved.end()) {
                report.add_error("Table name '" + table + "' is a reserved SQL keyword");
            }
        }
        
        return report;
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Naming convention validation failed: " + std::string(e.what()));
    }
}

DatabaseResult<ValidationReport> SchemaValidator::validate_foreign_keys() {
    ValidationReport report;
    
    try {
        auto tables = get_table_names();
        
        for (const auto& table : tables) {
            auto schema_info = get_table_schema(table);
            auto fk_list = extract_foreign_keys(schema_info["sql"]);
            
            for (const auto& fk : fk_list) {
                // Parse foreign key reference using boost::algorithm
                std::vector<std::string> parts;
                boost::algorithm::split(parts, fk, boost::algorithm::is_any_of(" "));
                
                if (parts.size() >= 3 && boost::algorithm::to_upper_copy(parts[0]) == "REFERENCES") {
                    std::string ref_table = parts[1];
                    boost::algorithm::replace_all(ref_table, "(", "");
                    boost::algorithm::replace_all(ref_table, ")", "");
                    
                    // Check if referenced table exists
                    if (std::find(tables.begin(), tables.end(), ref_table) == tables.end()) {
                        report.add_error("Foreign key in table '" + table + "' references non-existent table '" + ref_table + "'");
                    }
                }
            }
        }
        
        return report;
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Foreign key validation failed: " + std::string(e.what()));
    }
}

DatabaseResult<ValidationReport> SchemaValidator::validate_indexes() {
    ValidationReport report;
    
    try {
        auto stmt = db_.prepare(R"(
            SELECT name, tbl_name, sql 
            FROM sqlite_master 
            WHERE type='index' AND name NOT LIKE 'sqlite_%'
        )");
        
        if (!stmt) {
            return DatabaseError(stmt.error());
        }
        
        std::vector<std::tuple<std::string, std::string, std::string>> indexes;
        db_.execute_prepared(stmt.value(), [&indexes](const PreparedStatement& row) {
            auto name = std::get<std::string>(row.get_column(0));
            auto table = std::get<std::string>(row.get_column(1));
            auto sql = std::get<std::string>(row.get_column(2));
            indexes.emplace_back(name, table, sql);
        });
        
        for (const auto& [index_name, table_name, sql] : indexes) {
            if (!validate_index_naming(index_name, table_name)) {
                report.add_warning("Index '" + index_name + "' doesn't follow naming convention (idx_table_column)");
            }
        }
        
        return report;
        
    } catch (const std::exception& e) {
        return DatabaseError(DatabaseErrorType::InternalError, 
                           "Index validation failed: " + std::string(e.what()));
    }
}

void SchemaValidator::add_constraint(const SchemaConstraint& constraint) {
    constraints_.push_back(constraint);
}

std::vector<std::string> SchemaValidator::get_table_names() {
    auto stmt = db_.prepare(R"(
        SELECT name FROM sqlite_master 
        WHERE type='table' AND name NOT LIKE 'sqlite_%' AND name != 'schema_version'
        ORDER BY name
    )");
    
    if (!stmt) {
        return {};
    }
    
    std::vector<std::string> tables;
    db_.execute_prepared(stmt.value(), [&tables](const PreparedStatement& row) {
        tables.push_back(std::get<std::string>(row.get_column(0)));
    });
    
    return tables;
}

std::map<std::string, std::string> SchemaValidator::get_table_schema(const std::string& table_name) {
    auto stmt = db_.prepare("SELECT sql FROM sqlite_master WHERE type='table' AND name=?");
    if (!stmt) {
        return {};
    }
    
    stmt.value()->bind(1, table_name);
    
    std::map<std::string, std::string> schema;
    db_.execute_prepared(stmt.value(), [&schema](const PreparedStatement& row) {
        schema["sql"] = std::get<std::string>(row.get_column(0));
    });
    
    return schema;
}

bool SchemaValidator::validate_table_name(const std::string& name) {
    // Table names should be lowercase with underscores
    if (name.empty()) return false;
    
    // Check if all characters are lowercase, digits, or underscores
    return std::all_of(name.begin(), name.end(), [](char c) {
        return std::islower(c) || std::isdigit(c) || c == '_';
    }) && std::isalpha(name[0]); // Must start with letter
}

bool SchemaValidator::validate_column_name(const std::string& name) {
    // Column names should be lowercase with underscores
    if (name.empty()) return false;
    
    return std::all_of(name.begin(), name.end(), [](char c) {
        return std::islower(c) || std::isdigit(c) || c == '_';
    }) && std::isalpha(name[0]); // Must start with letter
}

bool SchemaValidator::check_constraint_exists(const std::string& table, const std::string& constraint) {
    auto schema = get_table_schema(table);
    if (schema.empty()) return false;
    
    std::string upper_sql = boost::algorithm::to_upper_copy(schema["sql"]);
    std::string upper_constraint = boost::algorithm::to_upper_copy(constraint);
    
    return upper_sql.find(upper_constraint) != std::string::npos;
}

std::vector<std::string> SchemaValidator::extract_foreign_keys(const std::string& schema) {
    std::vector<std::string> foreign_keys;
    
    // Simple regex to find FOREIGN KEY or REFERENCES clauses
    std::string upper_schema = boost::algorithm::to_upper_copy(schema);
    size_t pos = 0;
    
    while ((pos = upper_schema.find("REFERENCES", pos)) != std::string::npos) {
        size_t end = upper_schema.find(",", pos);
        if (end == std::string::npos) {
            end = upper_schema.find(")", pos);
        }
        if (end == std::string::npos) {
            end = upper_schema.length();
        }
        
        std::string fk_clause = schema.substr(pos, end - pos);
        boost::algorithm::trim(fk_clause);
        foreign_keys.push_back(fk_clause);
        
        pos = end;
    }
    
    return foreign_keys;
}

bool SchemaValidator::validate_index_naming(const std::string& index_name, const std::string& table_name) {
    // Index naming convention: idx_table_column or uk_table_column for unique
    if (boost::algorithm::starts_with(index_name, "idx_" + table_name + "_") ||
        boost::algorithm::starts_with(index_name, "uk_" + table_name + "_") ||
        boost::algorithm::starts_with(index_name, "pk_" + table_name)) {
        return true;
    }
    
    return false;
}

} // namespace liarsdice::database