#ifndef LIARSDICE_MIGRATION_HPP
#define LIARSDICE_MIGRATION_HPP

#include "schema_version.hpp"
#include <string>
#include <functional>
#include <boost/filesystem.hpp>

namespace liarsdice::database {

// Forward declaration
class DatabaseManager;

/**
 * @brief Represents a database migration
 */
class Migration {
public:
    using MigrationFunc = std::function<void(DatabaseManager&)>;
    
    Migration(int version, const std::string& description, 
              const std::string& up_sql, const std::string& down_sql = "")
        : version_(version, description)
        , up_sql_(up_sql)
        , down_sql_(down_sql) {
        calculate_checksum();
    }
    
    Migration(int version, const std::string& description,
              MigrationFunc up_func, MigrationFunc down_func = nullptr)
        : version_(version, description)
        , up_function_(up_func)
        , down_function_(down_func) {
        calculate_checksum();
    }
    
    /**
     * @brief Get the schema version info
     */
    const SchemaVersion& get_version() const { return version_; }
    
    /**
     * @brief Apply the migration (upgrade)
     */
    void apply(DatabaseManager& db);
    
    /**
     * @brief Rollback the migration (downgrade)
     */
    void rollback(DatabaseManager& db);
    
    /**
     * @brief Check if this migration can be rolled back
     */
    bool is_reversible() const {
        return !down_sql_.empty() || down_function_ != nullptr;
    }
    
    /**
     * @brief Get the SQL for upgrade
     */
    const std::string& get_up_sql() const { return up_sql_; }
    
    /**
     * @brief Get the SQL for downgrade
     */
    const std::string& get_down_sql() const { return down_sql_; }
    
private:
    SchemaVersion version_;
    std::string up_sql_;
    std::string down_sql_;
    MigrationFunc up_function_;
    MigrationFunc down_function_;
    
    void calculate_checksum();
};

/**
 * @brief Load migration from file
 */
class FileMigration : public Migration {
public:
    FileMigration(int version, const boost::filesystem::path& up_file,
                  const boost::filesystem::path& down_file = boost::filesystem::path());
    
private:
    static std::string read_file(const boost::filesystem::path& path);
};

} // namespace liarsdice::database

#endif // LIARSDICE_MIGRATION_HPP