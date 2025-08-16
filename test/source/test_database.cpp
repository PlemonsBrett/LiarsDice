#define BOOST_TEST_MODULE DatabaseTests
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <liarsdice/database/database_config.hpp>
#include <liarsdice/database/sqlite_extensions.hpp>
#include <sqlite3.h>

namespace fs = boost::filesystem;
using namespace liarsdice::database;

BOOST_AUTO_TEST_SUITE(DatabaseConfigTests)

BOOST_AUTO_TEST_CASE(DatabaseConfigSingleton) {
    // Test singleton pattern
    auto& config1 = DatabaseConfig::instance();
    auto& config2 = DatabaseConfig::instance();
    
    BOOST_CHECK_EQUAL(&config1, &config2);
}

BOOST_AUTO_TEST_CASE(DatabasePathManagement) {
    auto& config = DatabaseConfig::instance();
    
    // Create a temporary test directory
    fs::path temp_dir = fs::temp_directory_path() / "liarsdice_test_db";
    
    // Clean up if exists
    if (fs::exists(temp_dir)) {
        fs::remove_all(temp_dir);
    }
    
    // Set database directory
    config.set_database_directory(temp_dir);
    
    // Verify directory was created
    BOOST_CHECK(fs::exists(temp_dir));
    BOOST_CHECK(fs::is_directory(temp_dir));
    
    // Check database file path
    fs::path db_path = config.get_database_file_path();
    BOOST_CHECK_EQUAL(db_path.filename().string(), "liarsdice.db");
    // Use canonical path for comparison due to symlinks on macOS
    BOOST_CHECK_EQUAL(fs::canonical(db_path.parent_path()), fs::canonical(temp_dir));
    
    // Check backup file path
    fs::path backup_path = config.get_backup_file_path("test");
    BOOST_CHECK_EQUAL(backup_path.filename().string(), "liarsdice_backup_test.db");
    
    // Clean up
    fs::remove_all(temp_dir);
}

BOOST_AUTO_TEST_CASE(ConnectionStringGeneration) {
    auto& config = DatabaseConfig::instance();
    
    fs::path temp_dir = fs::temp_directory_path() / "liarsdice_test_db2";
    config.set_database_directory(temp_dir);
    
    std::string conn_str = config.get_connection_string();
    
    // Verify connection string format
    BOOST_CHECK(conn_str.find("file:") == 0);
    BOOST_CHECK(conn_str.find("mode=rwc") != std::string::npos);
    BOOST_CHECK(conn_str.find("cache=shared") != std::string::npos);
    BOOST_CHECK(conn_str.find("psow=1") != std::string::npos);
    BOOST_CHECK(conn_str.find("nolock=0") != std::string::npos);
    
    // Clean up
    fs::remove_all(temp_dir);
}

BOOST_AUTO_TEST_CASE(ThreadPoolConfiguration) {
    auto& config = DatabaseConfig::instance();
    
    // Test default thread pool size
    BOOST_CHECK_GT(config.get_thread_pool_size(), 0);
    
    // Test setting thread pool size
    config.set_thread_pool_size(8);
    BOOST_CHECK_EQUAL(config.get_thread_pool_size(), 8);
    
    // Test that zero is corrected to 1
    config.set_thread_pool_size(0);
    BOOST_CHECK_EQUAL(config.get_thread_pool_size(), 1);
}

BOOST_AUTO_TEST_SUITE_END() // DatabaseConfigTests

BOOST_AUTO_TEST_SUITE(SQLiteIntegrationTests)

BOOST_AUTO_TEST_CASE(SQLiteVersionCheck) {
    // Check SQLite version
    const char* version = sqlite3_libversion();
    BOOST_TEST_MESSAGE("SQLite version: " << version);
    
    int version_number = sqlite3_libversion_number();
    BOOST_CHECK_GE(version_number, 3008000); // Require at least 3.8.0
}

BOOST_AUTO_TEST_CASE(SQLiteThreadSafetyCheck) {
    // Check SQLite thread safety compilation
    int threadsafe = sqlite3_threadsafe();
    BOOST_TEST_MESSAGE("SQLite threadsafe mode: " << threadsafe);
    
    // We configured SQLITE_THREADSAFE=2 (multi-threaded mode)
    BOOST_CHECK_GT(threadsafe, 0);
}

BOOST_AUTO_TEST_CASE(BasicSQLiteConnection) {
    auto& config = DatabaseConfig::instance();
    
    // Create temporary database
    fs::path temp_dir = fs::temp_directory_path() / "liarsdice_test_db3";
    config.set_database_directory(temp_dir);
    
    // Open database connection
    sqlite3* db = nullptr;
    int result = sqlite3_open(config.get_database_file_path().string().c_str(), &db);
    BOOST_CHECK_EQUAL(result, SQLITE_OK);
    BOOST_CHECK(db != nullptr);
    
    if (db) {
        // Test simple query
        const char* sql = "SELECT sqlite_version()";
        sqlite3_stmt* stmt = nullptr;
        
        result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        BOOST_CHECK_EQUAL(result, SQLITE_OK);
        
        if (stmt) {
            result = sqlite3_step(stmt);
            BOOST_CHECK_EQUAL(result, SQLITE_ROW);
            
            const char* version = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            BOOST_TEST_MESSAGE("SQLite version from query: " << version);
            
            sqlite3_finalize(stmt);
        }
        
        sqlite3_close(db);
    }
    
    // Clean up
    fs::remove_all(temp_dir);
}

BOOST_AUTO_TEST_CASE(SQLiteFeatureCheck) {
    sqlite3* db = nullptr;
    int result = sqlite3_open(":memory:", &db);
    BOOST_REQUIRE_EQUAL(result, SQLITE_OK);
    BOOST_REQUIRE(db != nullptr);
    
    // Test JSON support (enabled by SQLITE_ENABLE_JSON1)
    const char* json_test = "SELECT json('{\"test\": 123}')";
    sqlite3_stmt* stmt = nullptr;
    
    result = sqlite3_prepare_v2(db, json_test, -1, &stmt, nullptr);
    if (result == SQLITE_OK && stmt) {
        BOOST_TEST_MESSAGE("JSON support is enabled");
        sqlite3_finalize(stmt);
    }
    
    // Test FTS5 support (enabled by SQLITE_ENABLE_FTS5)
    const char* fts5_test = "CREATE VIRTUAL TABLE test_fts USING fts5(content)";
    result = sqlite3_exec(db, fts5_test, nullptr, nullptr, nullptr);
    if (result == SQLITE_OK) {
        BOOST_TEST_MESSAGE("FTS5 support is enabled");
        sqlite3_exec(db, "DROP TABLE test_fts", nullptr, nullptr, nullptr);
    }
    
    sqlite3_close(db);
}

BOOST_AUTO_TEST_SUITE_END() // SQLiteIntegrationTests

BOOST_AUTO_TEST_SUITE(ExtensionManagerTests)

BOOST_AUTO_TEST_CASE(ExtensionManagerCreation) {
    SQLiteExtensionManager manager;
    
    // Test adding extension paths
    fs::path test_path = fs::temp_directory_path() / "test_extensions";
    if (!fs::exists(test_path)) {
        fs::create_directories(test_path);
    }
    
    manager.add_extension_path(test_path);
    
    // Test registering non-existent extension
    bool found = manager.register_extension("nonexistent", false);
    BOOST_CHECK(!found);
    
    // Test that required extension throws
    BOOST_CHECK_THROW(
        manager.register_extension("nonexistent", true),
        std::runtime_error
    );
    
    // Clean up
    fs::remove_all(test_path);
}

BOOST_AUTO_TEST_CASE(ExtensionLoadingCheck) {
    SQLiteExtensionManager manager;
    
    // Check that no extensions are loaded initially
    auto loaded = manager.get_loaded_extensions();
    BOOST_CHECK(loaded.empty());
    
    // Test is_extension_loaded for non-existent extension
    BOOST_CHECK(!manager.is_extension_loaded("test"));
}

BOOST_AUTO_TEST_SUITE_END() // ExtensionManagerTests