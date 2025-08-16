#define BOOST_TEST_MODULE DatabaseManagerSimpleTests
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <liarsdice/database/database_config.hpp>
#include <liarsdice/database/database_connection.hpp>
#include <liarsdice/database/database_manager.hpp>

namespace fs = boost::filesystem;
using namespace liarsdice::database;

// Simple fixture that bypasses ConnectionManager
struct SimpleDbFixture {
  SimpleDbFixture() {
    // Set up test database
    test_dir = fs::temp_directory_path()
               / ("liarsdice_test_simple_"
                  + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

    fs::create_directories(test_dir);
    db_path = test_dir / "test.db";
  }

  ~SimpleDbFixture() {
    if (fs::exists(test_dir)) {
      try {
        fs::remove_all(test_dir);
      } catch (...) {
      }
    }
  }

  fs::path test_dir;
  fs::path db_path;
};

BOOST_FIXTURE_TEST_SUITE(DatabaseManagerSimpleTests, SimpleDbFixture)

BOOST_AUTO_TEST_CASE(DirectConnectionTest) {
  // Test direct database connection without ConnectionManager
  DatabaseConnection conn;
  auto error = conn.open(db_path.string());

  BOOST_CHECK(!error);
  BOOST_CHECK(conn.is_open());
  BOOST_CHECK(fs::exists(db_path));

  // Try to create a table
  error = conn.execute("CREATE TABLE test (id INTEGER PRIMARY KEY)");
  BOOST_CHECK(!error);

  conn.close();
}

BOOST_AUTO_TEST_CASE(DirectConnectionWithURITest) {
  // Test with URI format
  std::string uri = "file:" + db_path.string() + "?mode=rwc";

  DatabaseConnection conn;
  auto error = conn.open(uri);

  BOOST_CHECK(!error);
  BOOST_CHECK(conn.is_open());

  error = conn.execute("CREATE TABLE test2 (id INTEGER PRIMARY KEY)");
  BOOST_CHECK(!error);

  conn.close();
}

BOOST_AUTO_TEST_SUITE_END()