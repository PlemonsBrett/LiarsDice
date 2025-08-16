#ifndef LIARSDICE_DATABASE_INITIALIZER_HPP
#define LIARSDICE_DATABASE_INITIALIZER_HPP

#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <liarsdice/database/database_error.hpp>
#include <liarsdice/database/database_manager.hpp>
#include <liarsdice/database/schema_manager.hpp>
#include <map>
#include <string>
#include <vector>

namespace liarsdice::database {

  /**
   * @brief Database initialization configuration structure
   */
  struct DatabaseInitConfig {
    bool create_schema = true;
    bool seed_data = true;
    bool run_migrations = true;
    bool validate_schema = false;
    std::string migration_directory;
    std::string seed_data_file;
    std::vector<std::string> required_tables;
    std::map<std::string, std::string> default_settings;

    // Performance settings
    int cache_size = 10000;
    int page_size = 4096;
    std::string journal_mode = "WAL";
    std::string synchronous = "NORMAL";

    // Feature flags
    bool enable_foreign_keys = true;
    bool enable_triggers = true;
    bool enable_full_text_search = false;
  };

  /**
   * @brief Seed data entry for database initialization
   */
  struct SeedDataEntry {
    std::string table_name;
    std::map<std::string, std::string> values;
    std::string condition;   // Optional WHERE clause for updates
    bool is_update = false;  // Insert or update operation
  };

  /**
   * @brief Database initializer using boost::property_tree for configuration
   */
  class DatabaseInitializer {
  public:
    explicit DatabaseInitializer(DatabaseManager& db, SchemaManager& schema_mgr);

    /**
     * @brief Initialize database from JSON configuration file
     */
    DatabaseResult<void> initialize_from_json(const boost::filesystem::path& config_file);

    /**
     * @brief Initialize database from XML configuration file
     */
    DatabaseResult<void> initialize_from_xml(const boost::filesystem::path& config_file);

    /**
     * @brief Initialize database from configuration object
     */
    DatabaseResult<void> initialize_from_config(const DatabaseInitConfig& config);

    /**
     * @brief Load seed data from JSON file
     */
    DatabaseResult<std::vector<SeedDataEntry>> load_seed_data_json(
        const boost::filesystem::path& data_file);

    /**
     * @brief Apply seed data entries to database
     */
    DatabaseResult<void> apply_seed_data(const std::vector<SeedDataEntry>& seed_data);

    /**
     * @brief Create default configuration template
     */
    DatabaseResult<void> create_config_template(const boost::filesystem::path& output_file,
                                                const std::string& format = "json");

    /**
     * @brief Validate database initialization requirements
     */
    DatabaseResult<void> validate_initialization_requirements(const DatabaseInitConfig& config);

    /**
     * @brief Configure database performance settings
     */
    DatabaseResult<void> configure_database_settings(const DatabaseInitConfig& config);

    /**
     * @brief Initialize default game data (achievements, settings, etc.)
     */
    DatabaseResult<void> initialize_game_defaults();

  private:
    DatabaseManager& db_;
    SchemaManager& schema_mgr_;

    DatabaseResult<DatabaseInitConfig> parse_json_config(const boost::property_tree::ptree& pt);
    DatabaseResult<DatabaseInitConfig> parse_xml_config(const boost::property_tree::ptree& pt);
    DatabaseResult<void> execute_pragma_settings(const DatabaseInitConfig& config);
    DatabaseResult<void> create_required_tables(const std::vector<std::string>& tables);
    DatabaseResult<void> seed_achievements_data();
    DatabaseResult<void> seed_default_settings();
    std::string escape_sql_value(const std::string& value);
  };

}  // namespace liarsdice::database

#endif  // LIARSDICE_DATABASE_INITIALIZER_HPP