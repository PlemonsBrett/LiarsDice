#define BOOST_TEST_MODULE DatabaseConnectionTests
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <liarsdice/database/database_connection.hpp>

namespace fs = boost::filesystem;
using namespace liarsdice::database;

BOOST_AUTO_TEST_SUITE(DatabaseConnectionTests)

BOOST_AUTO_TEST_CASE(BasicConnectionTest) {
  DatabaseConnection conn;

  // Test initial state
  BOOST_CHECK(!conn.is_open());
  BOOST_CHECK_EQUAL(static_cast<int>(conn.get_state()),
                    static_cast<int>(DatabaseConnection::State::Disconnected));
  BOOST_CHECK(conn.get() == nullptr);

  // Open in-memory database
  auto error = conn.open(":memory:");
  BOOST_CHECK(!error);
  BOOST_CHECK(conn.is_open());
  BOOST_CHECK_EQUAL(static_cast<int>(conn.get_state()),
                    static_cast<int>(DatabaseConnection::State::Connected));
  BOOST_CHECK(conn.get() != nullptr);

  // Test execute
  error = conn.execute("CREATE TABLE test (id INTEGER PRIMARY KEY, value TEXT)");
  BOOST_CHECK(!error);

  // Test insert
  error = conn.execute("INSERT INTO test (value) VALUES ('test_value')");
  BOOST_CHECK(!error);
  BOOST_CHECK_EQUAL(conn.get_last_insert_rowid(), 1);
  BOOST_CHECK_EQUAL(conn.get_changes(), 1);

  // Close connection
  conn.close();
  BOOST_CHECK(!conn.is_open());
  BOOST_CHECK_EQUAL(static_cast<int>(conn.get_state()),
                    static_cast<int>(DatabaseConnection::State::Disconnected));
}

BOOST_AUTO_TEST_CASE(TransactionTest) {
  DatabaseConnection conn;
  conn.open(":memory:");

  // Create table
  conn.execute("CREATE TABLE test (id INTEGER PRIMARY KEY, value INTEGER)");

  // Begin transaction
  auto error = conn.begin_transaction();
  BOOST_CHECK(!error);

  // Insert data
  conn.execute("INSERT INTO test (value) VALUES (10)");
  conn.execute("INSERT INTO test (value) VALUES (20)");

  // Rollback
  error = conn.rollback();
  BOOST_CHECK(!error);

  // Verify rollback
  sqlite3_stmt* stmt = nullptr;
  sqlite3_prepare_v2(conn.get(), "SELECT COUNT(*) FROM test", -1, &stmt, nullptr);
  sqlite3_step(stmt);
  int count = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  BOOST_CHECK_EQUAL(count, 0);

  // Test commit
  conn.begin_transaction();
  conn.execute("INSERT INTO test (value) VALUES (30)");
  error = conn.commit();
  BOOST_CHECK(!error);

  // Verify commit
  sqlite3_prepare_v2(conn.get(), "SELECT COUNT(*) FROM test", -1, &stmt, nullptr);
  sqlite3_step(stmt);
  count = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  BOOST_CHECK_EQUAL(count, 1);
}

BOOST_AUTO_TEST_CASE(ErrorHandlingTest) {
  DatabaseConnection conn;
  conn.open(":memory:");

  // Test invalid SQL
  auto error = conn.execute("INVALID SQL STATEMENT");
  BOOST_CHECK(error);
  BOOST_CHECK(!conn.get_last_error().empty());

  // Connection should still be valid
  BOOST_CHECK(conn.is_open());

  // Test valid operation after error
  error = conn.execute("CREATE TABLE test (id INTEGER)");
  BOOST_CHECK(!error);
}

BOOST_AUTO_TEST_SUITE_END()  // DatabaseConnectionTests
