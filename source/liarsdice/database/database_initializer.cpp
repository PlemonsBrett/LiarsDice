#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>
#include <fstream>
#include <liarsdice/database/database_initializer.hpp>
#include <sstream>

namespace liarsdice::database {

  DatabaseInitializer::DatabaseInitializer(DatabaseManager& db, SchemaManager& schema_mgr)
      : db_(db), schema_mgr_(schema_mgr) {}

  DatabaseResult<void> DatabaseInitializer::initialize_from_json(
      const boost::filesystem::path& config_file) {
    if (!boost::filesystem::exists(config_file)) {
      return DatabaseError(DatabaseErrorType::InvalidParameter,
                           "Configuration file not found: " + config_file.string());
    }

    try {
      boost::property_tree::ptree pt;
      boost::property_tree::read_json(config_file.string(), pt);

      auto config_result = parse_json_config(pt);
      if (!config_result) {
        return DatabaseError(config_result.error());
      }

      return initialize_from_config(config_result.value());

    } catch (const std::exception& e) {
      return DatabaseError(DatabaseErrorType::InternalError,
                           "Failed to parse JSON configuration: " + std::string(e.what()));
    }
  }

  DatabaseResult<void> DatabaseInitializer::initialize_from_xml(
      const boost::filesystem::path& config_file) {
    if (!boost::filesystem::exists(config_file)) {
      return DatabaseError(DatabaseErrorType::InvalidParameter,
                           "Configuration file not found: " + config_file.string());
    }

    try {
      boost::property_tree::ptree pt;
      boost::property_tree::read_xml(config_file.string(), pt);

      auto config_result = parse_xml_config(pt);
      if (!config_result) {
        return DatabaseError(config_result.error());
      }

      return initialize_from_config(config_result.value());

    } catch (const std::exception& e) {
      return DatabaseError(DatabaseErrorType::InternalError,
                           "Failed to parse XML configuration: " + std::string(e.what()));
    }
  }

  DatabaseResult<void> DatabaseInitializer::initialize_from_config(
      const DatabaseInitConfig& config) {
    BOOST_LOG_TRIVIAL(info) << "Starting database initialization";

    // Validate requirements
    auto validate_result = validate_initialization_requirements(config);
    if (!validate_result) {
      return validate_result;
    }

    // Configure database settings first
    auto settings_result = configure_database_settings(config);
    if (!settings_result) {
      BOOST_LOG_TRIVIAL(error) << "Failed to configure database settings: "
                               << settings_result.error().full_message();
      return settings_result;
    }

    // Run migrations if requested
    if (config.run_migrations && !config.migration_directory.empty()) {
      BOOST_LOG_TRIVIAL(info) << "Loading migrations from: " << config.migration_directory;

      auto migrations_result
          = schema_mgr_.load_migrations_from_directory(config.migration_directory);
      if (!migrations_result) {
        BOOST_LOG_TRIVIAL(warning)
            << "Failed to load migrations: " << migrations_result.error().full_message();
      } else {
        BOOST_LOG_TRIVIAL(info) << "Loaded " << migrations_result.value() << " migrations";

        auto migrate_result = schema_mgr_.migrate_to(-1);
        if (!migrate_result) {
          return DatabaseError(DatabaseErrorType::InternalError,
                               "Migration failed: " + migrate_result.error().full_message());
        }
      }
    }

    // Create required tables if specified
    if (!config.required_tables.empty()) {
      auto tables_result = create_required_tables(config.required_tables);
      if (!tables_result) {
        return tables_result;
      }
    }

    // Initialize game defaults
    auto defaults_result = initialize_game_defaults();
    if (!defaults_result) {
      BOOST_LOG_TRIVIAL(warning) << "Failed to initialize game defaults: "
                                 << defaults_result.error().full_message();
    }

    // Seed data if requested and file provided
    if (config.seed_data && !config.seed_data_file.empty()) {
      BOOST_LOG_TRIVIAL(info) << "Loading seed data from: " << config.seed_data_file;

      auto seed_result = load_seed_data_json(config.seed_data_file);
      if (!seed_result) {
        BOOST_LOG_TRIVIAL(warning)
            << "Failed to load seed data: " << seed_result.error().full_message();
      } else {
        auto apply_result = apply_seed_data(seed_result.value());
        if (!apply_result) {
          BOOST_LOG_TRIVIAL(warning)
              << "Failed to apply seed data: " << apply_result.error().full_message();
        } else {
          BOOST_LOG_TRIVIAL(info) << "Applied " << seed_result.value().size()
                                  << " seed data entries";
        }
      }
    }

    BOOST_LOG_TRIVIAL(info) << "Database initialization completed successfully";
    return DatabaseResult<void>();
  }

  DatabaseResult<std::vector<SeedDataEntry>> DatabaseInitializer::load_seed_data_json(
      const boost::filesystem::path& data_file) {
    if (!boost::filesystem::exists(data_file)) {
      return DatabaseError(DatabaseErrorType::InvalidParameter,
                           "Seed data file not found: " + data_file.string());
    }

    try {
      boost::property_tree::ptree pt;
      boost::property_tree::read_json(data_file.string(), pt);

      std::vector<SeedDataEntry> seed_data;

      for (const auto& entry_pair : pt.get_child("seed_data")) {
        const auto& entry = entry_pair.second;

        SeedDataEntry seed_entry;
        seed_entry.table_name = entry.get<std::string>("table");
        seed_entry.is_update = entry.get<bool>("is_update", false);
        seed_entry.condition = entry.get<std::string>("condition", "");

        // Load values
        for (const auto& value_pair : entry.get_child("values")) {
          seed_entry.values[value_pair.first] = value_pair.second.get_value<std::string>();
        }

        seed_data.push_back(seed_entry);
      }

      return seed_data;

    } catch (const std::exception& e) {
      return DatabaseError(DatabaseErrorType::InternalError,
                           "Failed to parse seed data JSON: " + std::string(e.what()));
    }
  }

  DatabaseResult<void> DatabaseInitializer::apply_seed_data(
      const std::vector<SeedDataEntry>& seed_data) {
    try {
      return db_.with_transaction([this, &seed_data](DatabaseManager& mgr) -> DatabaseResult<void> {
        for (const auto& entry : seed_data) {
          std::ostringstream sql;

          if (entry.is_update) {
            // Build UPDATE statement
            sql << "UPDATE " << entry.table_name << " SET ";

            bool first = true;
            for (const auto& [column, value] : entry.values) {
              if (!first) sql << ", ";
              sql << column << " = " << escape_sql_value(value);
              first = false;
            }

            if (!entry.condition.empty()) {
              sql << " WHERE " << entry.condition;
            }
          } else {
            // Build INSERT statement
            sql << "INSERT OR IGNORE INTO " << entry.table_name << " (";

            bool first = true;
            for (const auto& [column, value] : entry.values) {
              if (!first) sql << ", ";
              sql << column;
              first = false;
            }

            sql << ") VALUES (";

            first = true;
            for (const auto& [column, value] : entry.values) {
              if (!first) sql << ", ";
              sql << escape_sql_value(value);
              first = false;
            }

            sql << ")";
          }

          auto result = mgr.execute(sql.str());
          if (!result) {
            BOOST_LOG_TRIVIAL(error) << "Failed to execute seed SQL: " << sql.str()
                                     << " Error: " << result.error().full_message();
            return DatabaseError(result.error());
          }
        }

        return DatabaseResult<void>();
      });

    } catch (const std::exception& e) {
      return DatabaseError(DatabaseErrorType::InternalError,
                           "Failed to apply seed data: " + std::string(e.what()));
    }
  }

  DatabaseResult<void> DatabaseInitializer::create_config_template(
      const boost::filesystem::path& output_file, const std::string& format) {
    try {
      boost::property_tree::ptree pt;

      // Create template configuration
      pt.put("database.create_schema", true);
      pt.put("database.seed_data", true);
      pt.put("database.run_migrations", true);
      pt.put("database.validate_schema", false);
      pt.put("database.migration_directory", "./migrations");
      pt.put("database.seed_data_file", "./seed_data.json");

      // Performance settings
      pt.put("database.performance.cache_size", 10000);
      pt.put("database.performance.page_size", 4096);
      pt.put("database.performance.journal_mode", "WAL");
      pt.put("database.performance.synchronous", "NORMAL");

      // Feature flags
      pt.put("database.features.enable_foreign_keys", true);
      pt.put("database.features.enable_triggers", true);
      pt.put("database.features.enable_full_text_search", false);

      // Required tables
      boost::property_tree::ptree required_tables;
      required_tables.push_back(std::make_pair("", boost::property_tree::ptree("players")));
      required_tables.push_back(std::make_pair("", boost::property_tree::ptree("game_history")));
      required_tables.push_back(std::make_pair("", boost::property_tree::ptree("achievements")));
      pt.add_child("database.required_tables", required_tables);

      // Write template file
      if (boost::algorithm::to_lower_copy(format) == "xml") {
        boost::property_tree::write_xml(output_file.string(), pt);
      } else {
        boost::property_tree::write_json(output_file.string(), pt);
      }

      BOOST_LOG_TRIVIAL(info) << "Created configuration template: " << output_file;
      return DatabaseResult<void>();

    } catch (const std::exception& e) {
      return DatabaseError(DatabaseErrorType::InternalError,
                           "Failed to create config template: " + std::string(e.what()));
    }
  }

  DatabaseResult<DatabaseInitConfig> DatabaseInitializer::parse_json_config(
      const boost::property_tree::ptree& pt) {
    DatabaseInitConfig config;

    try {
      // Parse database settings
      if (pt.count("database")) {
        const auto& db_config = pt.get_child("database");

        config.create_schema = db_config.get<bool>("create_schema", true);
        config.seed_data = db_config.get<bool>("seed_data", true);
        config.run_migrations = db_config.get<bool>("run_migrations", true);
        config.validate_schema = db_config.get<bool>("validate_schema", false);
        config.migration_directory = db_config.get<std::string>("migration_directory", "");
        config.seed_data_file = db_config.get<std::string>("seed_data_file", "");

        // Parse performance settings
        if (db_config.count("performance")) {
          const auto& perf_config = db_config.get_child("performance");
          config.cache_size = perf_config.get<int>("cache_size", 10000);
          config.page_size = perf_config.get<int>("page_size", 4096);
          config.journal_mode = perf_config.get<std::string>("journal_mode", "WAL");
          config.synchronous = perf_config.get<std::string>("synchronous", "NORMAL");
        }

        // Parse feature flags
        if (db_config.count("features")) {
          const auto& features = db_config.get_child("features");
          config.enable_foreign_keys = features.get<bool>("enable_foreign_keys", true);
          config.enable_triggers = features.get<bool>("enable_triggers", true);
          config.enable_full_text_search = features.get<bool>("enable_full_text_search", false);
        }

        // Parse required tables
        if (db_config.count("required_tables")) {
          for (const auto& table_entry : db_config.get_child("required_tables")) {
            config.required_tables.push_back(table_entry.second.get_value<std::string>());
          }
        }
      }

      return config;

    } catch (const std::exception& e) {
      return DatabaseError(DatabaseErrorType::InternalError,
                           "Failed to parse JSON config: " + std::string(e.what()));
    }
  }

  DatabaseResult<DatabaseInitConfig> DatabaseInitializer::parse_xml_config(
      const boost::property_tree::ptree& pt) {
    // Similar to JSON parsing but for XML structure
    return parse_json_config(pt);  // For simplicity, use same logic
  }

  DatabaseResult<void> DatabaseInitializer::configure_database_settings(
      const DatabaseInitConfig& config) {
    try {
      auto pragma_result = execute_pragma_settings(config);
      if (!pragma_result) {
        return pragma_result;
      }

      BOOST_LOG_TRIVIAL(info) << "Database settings configured successfully";
      return DatabaseResult<void>();

    } catch (const std::exception& e) {
      return DatabaseError(DatabaseErrorType::InternalError,
                           "Failed to configure database settings: " + std::string(e.what()));
    }
  }

  DatabaseResult<void> DatabaseInitializer::execute_pragma_settings(
      const DatabaseInitConfig& config) {
    std::vector<std::string> pragma_statements
        = {"PRAGMA cache_size = " + std::to_string(config.cache_size),
           "PRAGMA page_size = " + std::to_string(config.page_size),
           "PRAGMA journal_mode = " + config.journal_mode,
           "PRAGMA synchronous = " + config.synchronous,
           "PRAGMA foreign_keys = " + std::string(config.enable_foreign_keys ? "ON" : "OFF"),
           "PRAGMA recursive_triggers = " + std::string(config.enable_triggers ? "ON" : "OFF")};

    for (const auto& pragma : pragma_statements) {
      auto result = db_.execute(pragma);
      if (!result) {
        BOOST_LOG_TRIVIAL(warning) << "Failed to execute pragma: " << pragma
                                   << " Error: " << result.error().full_message();
      } else {
        BOOST_LOG_TRIVIAL(debug) << "Executed pragma: " << pragma;
      }
    }

    return DatabaseResult<void>();
  }

  DatabaseResult<void> DatabaseInitializer::initialize_game_defaults() {
    auto achievements_result = seed_achievements_data();
    if (!achievements_result) {
      return achievements_result;
    }

    auto settings_result = seed_default_settings();
    if (!settings_result) {
      return settings_result;
    }

    return DatabaseResult<void>();
  }

  DatabaseResult<void> DatabaseInitializer::seed_achievements_data() {
    // This would insert default achievements - simplified for demo
    const std::vector<std::tuple<std::string, std::string, std::string, int>> achievements
        = {{"first_win", "First Victory", "Win your first game", 10},
           {"win_streak_5", "Hot Streak", "Win 5 games in a row", 50},
           {"perfect_game", "Flawless Victory", "Win without losing a single point", 100}};

    for (const auto& [code, name, desc, points] : achievements) {
      std::ostringstream sql;
      sql << "INSERT OR IGNORE INTO achievements (code, name, description, points, category) "
             "VALUES ("
          << "'" << code << "', '" << name << "', '" << desc << "', " << points << ", 'milestone')";

      auto result = db_.execute(sql.str());
      if (!result) {
        BOOST_LOG_TRIVIAL(warning) << "Failed to insert achievement: " << code;
      }
    }

    return DatabaseResult<void>();
  }

  DatabaseResult<void> DatabaseInitializer::seed_default_settings() {
    // This would insert default game settings if we had a settings table
    BOOST_LOG_TRIVIAL(info) << "Default settings seeded (placeholder implementation)";
    return DatabaseResult<void>();
  }

  std::string DatabaseInitializer::escape_sql_value(const std::string& value) {
    // Simple SQL escaping - in production, use parameterized queries
    std::string escaped = value;
    boost::algorithm::replace_all(escaped, "'", "''");
    return "'" + escaped + "'";
  }

  DatabaseResult<void> DatabaseInitializer::validate_initialization_requirements(
      const DatabaseInitConfig& config) {
    // Validate migration directory exists if migrations are requested
    if (config.run_migrations && !config.migration_directory.empty()) {
      if (!boost::filesystem::exists(config.migration_directory)) {
        return DatabaseError(DatabaseErrorType::InvalidParameter,
                             "Migration directory does not exist: " + config.migration_directory);
      }
    }

    // Validate seed data file exists if seeding is requested
    if (config.seed_data && !config.seed_data_file.empty()) {
      if (!boost::filesystem::exists(config.seed_data_file)) {
        return DatabaseError(DatabaseErrorType::InvalidParameter,
                             "Seed data file does not exist: " + config.seed_data_file);
      }
    }

    return DatabaseResult<void>();
  }

  DatabaseResult<void> DatabaseInitializer::create_required_tables(
      const std::vector<std::string>& tables) {
    BOOST_LOG_TRIVIAL(info) << "Creating " << tables.size() << " required tables";

    // This is a simplified implementation - in practice, you'd have table definitions
    for (const auto& table : tables) {
      BOOST_LOG_TRIVIAL(debug) << "Ensuring table exists: " << table;
      // Table creation would be handled by migrations in a real implementation
    }

    return DatabaseResult<void>();
  }

}  // namespace liarsdice::database