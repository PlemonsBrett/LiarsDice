#ifndef LIARSDICE_DATABASE_SCHEMA_VALIDATOR_HPP
#define LIARSDICE_DATABASE_SCHEMA_VALIDATOR_HPP

#include <liarsdice/database/database_error.hpp>
#include <liarsdice/database/database_manager.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace liarsdice::database {

/**
 * @brief Schema constraint types for validation
 */
enum class ConstraintType {
    NotNull,
    Unique,
    PrimaryKey,
    ForeignKey,
    Check,
    Default,
    Index
};

/**
 * @brief Schema constraint definition
 */
struct SchemaConstraint {
    std::string table_name;
    std::string column_name;
    ConstraintType type;
    std::string constraint_name;
    std::string definition;
    std::vector<std::string> referenced_tables;
    std::function<bool(const std::string&)> validator;
    
    SchemaConstraint(const std::string& table, const std::string& column, 
                    ConstraintType constraint_type, const std::string& name = "",
                    const std::string& def = "")
        : table_name(table), column_name(column), type(constraint_type),
          constraint_name(name), definition(def) {}
};

/**
 * @brief Schema validation report
 */
struct ValidationReport {
    bool is_valid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::map<std::string, std::vector<std::string>> table_issues;
    
    void add_error(const std::string& error) {
        is_valid = false;
        errors.push_back(error);
    }
    
    void add_warning(const std::string& warning) {
        warnings.push_back(warning);
    }
    
    void add_table_issue(const std::string& table, const std::string& issue) {
        table_issues[table].push_back(issue);
        add_error("Table '" + table + "': " + issue);
    }
};

/**
 * @brief Schema validator using boost::algorithm for constraint checking
 */
class SchemaValidator {
public:
    explicit SchemaValidator(DatabaseManager& db) : db_(db) {
        initialize_constraints();
    }
    
    /**
     * @brief Validate entire database schema
     */
    DatabaseResult<ValidationReport> validate_schema();
    
    /**
     * @brief Validate specific table schema
     */
    DatabaseResult<ValidationReport> validate_table(const std::string& table_name);
    
    /**
     * @brief Add custom constraint for validation
     */
    void add_constraint(const SchemaConstraint& constraint);
    
    /**
     * @brief Check naming conventions using boost::algorithm
     */
    DatabaseResult<ValidationReport> validate_naming_conventions();
    
    /**
     * @brief Validate data types and constraints
     */
    DatabaseResult<ValidationReport> validate_data_integrity();
    
    /**
     * @brief Check foreign key relationships
     */
    DatabaseResult<ValidationReport> validate_foreign_keys();
    
    /**
     * @brief Validate indexes for performance
     */
    DatabaseResult<ValidationReport> validate_indexes();

private:
    DatabaseManager& db_;
    std::vector<SchemaConstraint> constraints_;
    
    void initialize_constraints();
    std::vector<std::string> get_table_names();
    std::map<std::string, std::string> get_table_schema(const std::string& table_name);
    bool validate_table_name(const std::string& name);
    bool validate_column_name(const std::string& name);
    bool check_constraint_exists(const std::string& table, const std::string& constraint);
    std::vector<std::string> extract_foreign_keys(const std::string& schema);
    bool validate_index_naming(const std::string& index_name, const std::string& table_name);
};

} // namespace liarsdice::database

#endif // LIARSDICE_DATABASE_SCHEMA_VALIDATOR_HPP