#define BOOST_TEST_MODULE DatabaseConnectionTests
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <liarsdice/database/database_connection.hpp>
#include <liarsdice/database/connection_pool.hpp>
#include <liarsdice/database/connection_manager.hpp>
#include <chrono>
#include <thread>
#include <iostream>

namespace fs = boost::filesystem;
using namespace liarsdice::database;

namespace boost { namespace test_tools { namespace tt_detail {
// Helper for printing enum state
template<>
struct print_log_value<liarsdice::database::DatabaseConnection::State> {
    void operator()(std::ostream& os, const liarsdice::database::DatabaseConnection::State& state) {
        switch (state) {
            case liarsdice::database::DatabaseConnection::State::Disconnected: os << "Disconnected"; break;
            case liarsdice::database::DatabaseConnection::State::Connected: os << "Connected"; break;
            case liarsdice::database::DatabaseConnection::State::Error: os << "Error"; break;
        }
    }
};
}}}

BOOST_AUTO_TEST_SUITE(DatabaseConnectionTests)

BOOST_AUTO_TEST_CASE(BasicConnectionTest) {
    DatabaseConnection conn;
    
    // Test initial state
    BOOST_CHECK(!conn.is_open());
    BOOST_CHECK_EQUAL(conn.get_state(), DatabaseConnection::State::Disconnected);
    BOOST_CHECK(conn.get() == nullptr);
    
    // Open in-memory database
    auto error = conn.open(":memory:");
    BOOST_CHECK(!error);
    BOOST_CHECK(conn.is_open());
    BOOST_CHECK_EQUAL(conn.get_state(), DatabaseConnection::State::Connected);
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
    BOOST_CHECK_EQUAL(conn.get_state(), DatabaseConnection::State::Disconnected);
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

BOOST_AUTO_TEST_CASE(IdleTimeTest) {
    DatabaseConnection conn;
    conn.open(":memory:");
    
    // Initial idle time should be near zero
    auto idle1 = conn.get_idle_time();
    BOOST_CHECK(idle1.count() < 1000000); // Less than 1ms in nanoseconds
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Idle time should increase
    auto idle2 = conn.get_idle_time();
    BOOST_CHECK(idle2 > idle1);
    
    // Activity should reset idle time
    conn.execute("SELECT 1");
    auto idle3 = conn.get_idle_time();
    BOOST_CHECK(idle3 < idle2);
}

BOOST_AUTO_TEST_SUITE_END() // DatabaseConnectionTests

BOOST_AUTO_TEST_SUITE(ConnectionPoolTests)

struct PoolFixture {
    boost::asio::io_context io_context;
    std::unique_ptr<boost::thread> io_thread;
    
    PoolFixture() {
        io_thread = std::make_unique<boost::thread>([this] {
            auto work = boost::asio::make_work_guard(io_context);
            io_context.run();
        });
    }
    
    ~PoolFixture() {
        io_context.stop();
        if (io_thread && io_thread->joinable()) {
            io_thread->join();
        }
    }
};

BOOST_FIXTURE_TEST_CASE(BasicPoolTest, PoolFixture) {
    ConnectionPool::PoolConfig config;
    config.min_connections = 2;
    config.max_connections = 5;
    config.enable_health_checks = false; // Disable for test
    
    ConnectionPool pool(io_context, ":memory:", config);
    
    // Check initial stats
    auto stats = pool.get_stats();
    BOOST_CHECK_GE(stats.total_connections, config.min_connections);
    BOOST_CHECK_GE(stats.available_connections, config.min_connections);
    BOOST_CHECK_EQUAL(stats.active_connections, 0);
    
    // Acquire connection
    {
        auto conn = pool.acquire();
        BOOST_CHECK(conn);
        BOOST_CHECK(conn->is_open());
        
        stats = pool.get_stats();
        BOOST_CHECK_EQUAL(stats.active_connections, 1);
        
        // Use connection
        auto error = conn->execute("CREATE TABLE test (id INTEGER)");
        BOOST_CHECK(!error);
    }
    
    // Connection should be returned to pool
    stats = pool.get_stats();
    BOOST_CHECK_EQUAL(stats.active_connections, 0);
    BOOST_CHECK_GE(stats.available_connections, 1);
}

BOOST_FIXTURE_TEST_CASE(PoolConcurrencyTest, PoolFixture) {
    ConnectionPool::PoolConfig config;
    config.min_connections = 2;
    config.max_connections = 10;
    config.enable_health_checks = false;
    
    ConnectionPool pool(io_context, ":memory:", config);
    
    const int num_threads = 5;
    const int ops_per_thread = 10;
    std::atomic<int> successful_ops(0);
    
    std::vector<boost::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&pool, &successful_ops, ops_per_thread] {
            for (int j = 0; j < ops_per_thread; ++j) {
                auto conn = pool.acquire(std::chrono::milliseconds(1000));
                if (conn && conn->is_open()) {
                    auto error = conn->execute("SELECT 1");
                    if (!error) {
                        ++successful_ops;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // All operations should succeed
    BOOST_CHECK_EQUAL(successful_ops.load(), num_threads * ops_per_thread);
    
    // Pool should be stable
    auto stats = pool.get_stats();
    BOOST_CHECK_LE(stats.total_connections, config.max_connections);
    BOOST_CHECK_EQUAL(stats.active_connections, 0);
}

BOOST_FIXTURE_TEST_CASE(PoolTimeoutTest, PoolFixture) {
    ConnectionPool::PoolConfig config;
    config.min_connections = 1;
    config.max_connections = 1;
    config.enable_health_checks = false;
    
    ConnectionPool pool(io_context, ":memory:", config);
    
    // Acquire the only connection
    auto conn1 = pool.acquire();
    BOOST_CHECK(conn1);
    
    // Try to acquire another with short timeout
    auto conn2 = pool.acquire(std::chrono::milliseconds(100));
    BOOST_CHECK(!conn2);
    
    // Release first connection
    conn1 = ConnectionPool::PooledConnection();
    
    // Now should be able to acquire
    auto conn3 = pool.acquire(std::chrono::milliseconds(100));
    BOOST_CHECK(conn3);
}

BOOST_AUTO_TEST_SUITE_END() // ConnectionPoolTests

BOOST_AUTO_TEST_SUITE(ConnectionManagerTests)

BOOST_AUTO_TEST_CASE(SingletonTest) {
    auto& manager1 = ConnectionManager::instance();
    auto& manager2 = ConnectionManager::instance();
    
    BOOST_CHECK_EQUAL(&manager1, &manager2);
}

BOOST_AUTO_TEST_CASE(BasicManagerTest) {
    auto& manager = ConnectionManager::instance();
    
    // Configure if not already done
    if (!manager.is_initialized()) {
        DatabaseConfig::instance().set_database_directory(fs::temp_directory_path() / "liarsdice_test_db_manager");
        
        ConnectionPool::PoolConfig config;
        config.min_connections = 2;
        config.max_connections = 5;
        config.enable_health_checks = false;
        
        manager.configure(config);
    }
    
    BOOST_CHECK(manager.is_initialized());
    
    // Acquire connection
    auto conn = manager.acquire_connection();
    BOOST_CHECK(conn);
    BOOST_CHECK(conn->is_open());
    
    // Use connection
    auto error = conn->execute("CREATE TEMP TABLE test (id INTEGER)");
    BOOST_CHECK(!error);
    
    // Check stats
    auto stats = manager.get_pool_stats();
    BOOST_CHECK_GE(stats.total_connections, 1);
}

BOOST_AUTO_TEST_CASE(TransactionHelperTest) {
    auto& manager = ConnectionManager::instance();
    
    if (!manager.is_initialized()) {
        ConnectionPool::PoolConfig config;
        config.min_connections = 1;
        config.max_connections = 3;
        config.enable_health_checks = false;
        manager.configure(config);
    }
    
    // Test successful transaction
    auto error = manager.execute_transaction([](DatabaseConnection& conn) {
        auto err = conn.execute("CREATE TEMP TABLE trans_test (id INTEGER)");
        if (err) return err;
        
        err = conn.execute("INSERT INTO trans_test VALUES (1), (2), (3)");
        return err;
    });
    
    BOOST_CHECK(!error);
    
    // Test failed transaction (should rollback)
    error = manager.execute_transaction([](DatabaseConnection& conn) {
        conn.execute("CREATE TEMP TABLE fail_test (id INTEGER UNIQUE)");
        conn.execute("INSERT INTO fail_test VALUES (1)");
        
        // This should fail due to unique constraint
        return conn.execute("INSERT INTO fail_test VALUES (1)");
    });
    
    BOOST_CHECK(error);
}

BOOST_AUTO_TEST_CASE(WithConnectionTest) {
    auto& manager = ConnectionManager::instance();
    
    if (!manager.is_initialized()) {
        ConnectionPool::PoolConfig config;
        config.min_connections = 1;
        config.max_connections = 3;
        config.enable_health_checks = false;
        manager.configure(config);
    }
    
    // Test with_connection helper
    int result = manager.with_connection([](DatabaseConnection& conn) {
        conn.execute("CREATE TEMP TABLE with_test (value INTEGER)");
        conn.execute("INSERT INTO with_test VALUES (42)");
        
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(conn.get(), "SELECT value FROM with_test", -1, &stmt, nullptr);
        sqlite3_step(stmt);
        int value = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        
        return value;
    });
    
    BOOST_CHECK_EQUAL(result, 42);
}

BOOST_AUTO_TEST_SUITE_END() // ConnectionManagerTests