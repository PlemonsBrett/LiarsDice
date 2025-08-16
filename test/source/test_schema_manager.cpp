#define BOOST_TEST_MODULE SchemaManagerTests
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <fstream>
#include <liarsdice/database/connection_manager.hpp>
#include <liarsdice/database/database_config.hpp>
#include <liarsdice/database/database_manager.hpp>
#include <liarsdice/database/schema_manager.hpp>

namespace fs = boost::filesystem;
using namespace liarsdice::database;

struct SchemaManagerFixture {
  SchemaManagerFixture() {
    // Set up test database
    test_dir = fs::temp_directory_path()
               / ("liarsdice_test_schema_"
                  + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

    fs::create_directories(test_dir);

    // Configure database
    auto& config = DatabaseConfig::instance();
    config.set_database_directory(test_dir);

    // Initialize ConnectionManager
    auto& conn_mgr = ConnectionManager::instance();
    if (!conn_mgr.is_initialized()) {
      ConnectionPool::PoolConfig pool_config;
      pool_config.min_connections = 1;
      pool_config.max_connections = 2;
      pool_config.enable_health_checks = false;
      conn_mgr.configure(pool_config);
    }

    // Create database manager and schema manager
    db_mgr = std::make_unique<DatabaseManager>();
    schema_mgr = std::make_unique<SchemaManager>(*db_mgr);

    // Create test migrations directory
    migrations_dir = test_dir / "migrations";
    fs::create_directories(migrations_dir);
  }

  ~SchemaManagerFixture() {
    // Clean up
    auto& conn_mgr = ConnectionManager::instance();
    conn_mgr.shutdown();

    if (fs::exists(test_dir)) {
      try {
        fs::remove_all(test_dir);
      } catch (...) {
      }
    }
  }

  void create_migration_file(const std::string& filename, const std::string& content) {
    std::ofstream file((migrations_dir / filename).string());
    file << content;
    file.close();
  }

  fs::path test_dir;
  fs::path migrations_dir;
  std::unique_ptr<DatabaseManager> db_mgr;
  std::unique_ptr<SchemaManager> schema_mgr;
};

BOOST_FIXTURE_TEST_SUITE(SchemaManagerTests, SchemaManagerFixture)

BOOST_AUTO_TEST_CASE(InitialVersionTest) {
  // Initially, there should be no version
  auto version = schema_mgr->get_current_version();
  BOOST_REQUIRE(version.has_value());
  BOOST_CHECK_EQUAL(version.value(), 0);

  // No migrations should be applied
  auto applied = schema_mgr->get_applied_migrations();
  BOOST_REQUIRE(applied.has_value());
  BOOST_CHECK(applied.value().empty());
}

BOOST_AUTO_TEST_CASE(AddMigrationTest) {
  // Create a simple migration
  auto migration = std::make_unique<Migration>(
      1, "Create test table", "CREATE TABLE test (id INTEGER PRIMARY KEY)", "DROP TABLE test");

  schema_mgr->add_migration(std::move(migration));

  // Check pending migrations
  auto pending = schema_mgr->get_pending_migrations();
  BOOST_REQUIRE(pending.has_value());
  BOOST_CHECK_EQUAL(pending.value().size(), 1);
  BOOST_CHECK_EQUAL(pending.value()[0]->get_version().version, 1);
}

BOOST_AUTO_TEST_CASE(MigrateUpTest) {
  // Add two migrations
  schema_mgr->add_migration(std::make_unique<Migration>(
      1, "Create table1", "CREATE TABLE table1 (id INTEGER PRIMARY KEY)", "DROP TABLE table1"));

  schema_mgr->add_migration(std::make_unique<Migration>(
      2, "Create table2", "CREATE TABLE table2 (id INTEGER PRIMARY KEY)", "DROP TABLE table2"));

  // Migrate to latest
  auto result = schema_mgr->migrate_to(-1);
  BOOST_REQUIRE(result.has_value());

  // Check current version
  auto version = schema_mgr->get_current_version();
  BOOST_REQUIRE(version.has_value());
  BOOST_CHECK_EQUAL(version.value(), 2);

  // Check applied migrations
  auto applied = schema_mgr->get_applied_migrations();
  BOOST_REQUIRE(applied.has_value());
  BOOST_CHECK_EQUAL(applied.value().size(), 2);

  // Verify tables exist
  auto check1 = db_mgr->execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name='table1'");
  BOOST_CHECK(check1.has_value());

  auto check2 = db_mgr->execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name='table2'");
  BOOST_CHECK(check2.has_value());
}

BOOST_AUTO_TEST_CASE(RollbackTest) {
  // Add reversible migrations
  schema_mgr->add_migration(std::make_unique<Migration>(
      1, "Create table1", "CREATE TABLE table1 (id INTEGER PRIMARY KEY)", "DROP TABLE table1"));

  schema_mgr->add_migration(std::make_unique<Migration>(
      2, "Create table2", "CREATE TABLE table2 (id INTEGER PRIMARY KEY)", "DROP TABLE table2"));

  // Migrate to version 2
  auto result = schema_mgr->migrate_to(2);
  BOOST_REQUIRE(result.has_value());

  // Rollback to version 1
  result = schema_mgr->rollback_to(1);
  BOOST_REQUIRE(result.has_value());

  // Check current version
  auto version = schema_mgr->get_current_version();
  BOOST_REQUIRE(version.has_value());
  BOOST_CHECK_EQUAL(version.value(), 1);

  // Verify table2 is gone but table1 exists
  auto stmt = db_mgr->prepare("SELECT name FROM sqlite_master WHERE type='table'");
  BOOST_REQUIRE(stmt.has_value());

  std::vector<std::string> tables;
  db_mgr->execute_prepared(stmt.value(), [&tables](const PreparedStatement& row) {
    tables.push_back(std::get<std::string>(row.get_column(0)));
  });

  BOOST_CHECK(std::find(tables.begin(), tables.end(), "table1") != tables.end());
  BOOST_CHECK(std::find(tables.begin(), tables.end(), "table2") == tables.end());
}

BOOST_AUTO_TEST_CASE(LoadFromDirectoryTest) {
  // Create migration files
  create_migration_file("V001__first.up.sql", "CREATE TABLE first_table (id INTEGER PRIMARY KEY)");
  create_migration_file("V001__first.down.sql", "DROP TABLE first_table");

  create_migration_file("V002__second.up.sql",
                        "CREATE TABLE second_table (id INTEGER PRIMARY KEY)");
  create_migration_file("V002__second.down.sql", "DROP TABLE second_table");

  // Load migrations
  auto count = schema_mgr->load_migrations_from_directory(migrations_dir);
  BOOST_REQUIRE(count.has_value());
  BOOST_CHECK_EQUAL(count.value(), 2);

  // Apply migrations
  auto result = schema_mgr->migrate_to(-1);
  BOOST_REQUIRE(result.has_value());

  // Verify version
  auto version = schema_mgr->get_current_version();
  BOOST_REQUIRE(version.has_value());
  BOOST_CHECK_EQUAL(version.value(), 2);
}

BOOST_AUTO_TEST_CASE(BaselineTest) {
  // Create a baseline at version 5
  auto result = schema_mgr->baseline(5, "Initial baseline");
  BOOST_REQUIRE(result.has_value());

  // Check current version
  auto version = schema_mgr->get_current_version();
  BOOST_REQUIRE(version.has_value());
  BOOST_CHECK_EQUAL(version.value(), 5);

  // Add migration 6
  schema_mgr->add_migration(std::make_unique<Migration>(
      6, "After baseline", "CREATE TABLE after_baseline (id INTEGER PRIMARY KEY)"));

  // Migrate
  result = schema_mgr->migrate_to(-1);
  BOOST_REQUIRE(result.has_value());

  // Check final version
  version = schema_mgr->get_current_version();
  BOOST_REQUIRE(version.has_value());
  BOOST_CHECK_EQUAL(version.value(), 6);
}

BOOST_AUTO_TEST_CASE(ValidationTest) {
  // Add migrations with gap
  schema_mgr->add_migration(
      std::make_unique<Migration>(1, "First", "CREATE TABLE t1 (id INTEGER)"));

  schema_mgr->add_migration(std::make_unique<Migration>(
      3, "Third", "CREATE TABLE t3 (id INTEGER)"  // Gap at version 2
      ));

  // Validation should fail
  auto result = schema_mgr->validate_migrations();
  BOOST_CHECK(!result.has_value());
  BOOST_CHECK(result.error().type() == DatabaseErrorType::InvalidParameter);
}

BOOST_AUTO_TEST_CASE(TransactionRollbackOnFailureTest) {
  // Add a migration that will fail
  schema_mgr->add_migration(
      std::make_unique<Migration>(1, "Create table", "CREATE TABLE test (id INTEGER PRIMARY KEY)"));

  schema_mgr->add_migration(
      std::make_unique<Migration>(2, "Invalid SQL", "CREATE TABLE INVALID SYNTAX HERE"));

  // Try to migrate - should fail and rollback
  auto result = schema_mgr->migrate_to(-1);
  BOOST_CHECK(!result.has_value());

  // Version should still be 1 (first migration succeeded before transaction)
  auto version = schema_mgr->get_current_version();
  BOOST_REQUIRE(version.has_value());
  BOOST_CHECK_EQUAL(version.value(), 1);
}

BOOST_AUTO_TEST_CASE(FunctionalMigrationTest) {
  // Create migration with function
  auto up_func = [](DatabaseManager& db) {
    auto result = db.execute("CREATE TABLE func_table (id INTEGER, name TEXT)");
    if (!result) {
      throw std::runtime_error("Failed to create table");
    }

    // Insert some data
    auto stmt = db.prepare("INSERT INTO func_table (id, name) VALUES (?, ?)");
    if (!stmt) {
      throw std::runtime_error("Failed to prepare statement");
    }

    stmt.value()->bind(1, 1);
    stmt.value()->bind(2, std::string("Test"));
    db.execute_prepared(stmt.value());
  };

  auto down_func = [](DatabaseManager& db) {
    auto result = db.execute("DROP TABLE func_table");
    if (!result) {
      throw std::runtime_error("Failed to drop table");
    }
  };

  schema_mgr->add_migration(
      std::make_unique<Migration>(1, "Functional migration", up_func, down_func));

  // Apply migration
  auto result = schema_mgr->migrate_to(1);
  BOOST_REQUIRE(result.has_value());

  // Verify data was inserted
  auto stmt = db_mgr->prepare("SELECT name FROM func_table WHERE id = 1");
  BOOST_REQUIRE(stmt.has_value());

  std::string name;
  db_mgr->execute_prepared(stmt.value(), [&name](const PreparedStatement& row) {
    name = std::get<std::string>(row.get_column(0));
  });

  BOOST_CHECK_EQUAL(name, "Test");
}

BOOST_AUTO_TEST_SUITE_END()