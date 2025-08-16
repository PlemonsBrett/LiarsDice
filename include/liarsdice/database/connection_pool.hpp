#ifndef LIARSDICE_CONNECTION_POOL_HPP
#define LIARSDICE_CONNECTION_POOL_HPP

#include "database_connection.hpp"
#include <boost/pool/object_pool.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/chrono.hpp>
#include <queue>
#include <memory>
#include <atomic>
#include <functional>

namespace liarsdice::database {

/**
 * @brief Thread-safe database connection pool
 * 
 * Manages a pool of database connections with automatic
 * connection recycling, health monitoring, and size management.
 */
class ConnectionPool : private boost::noncopyable {
public:
    using ConnectionPtr = std::shared_ptr<DatabaseConnection>;
    using ConnectionDeleter = std::function<void(DatabaseConnection*)>;
    
    struct PoolConfig {
        size_t min_connections;
        size_t max_connections;
        std::chrono::seconds idle_timeout;
        std::chrono::seconds health_check_interval;
        std::chrono::seconds connection_timeout;
        bool enable_health_checks;
        
        PoolConfig() 
            : min_connections(5)  // Increased from 2 for better concurrency
            , max_connections(20)  // Increased from 10 for higher loads
            , idle_timeout(300) // 5 minutes
            , health_check_interval(60) // 1 minute
            , connection_timeout(10)
            , enable_health_checks(true) {}
    };
    
    /**
     * @brief Connection wrapper for automatic return to pool
     */
    class PooledConnection {
    public:
        PooledConnection() = default;
        
        PooledConnection(ConnectionPtr conn, std::function<void(ConnectionPtr)> returner)
            : connection_(conn), returner_(returner) {}
        
        ~PooledConnection() {
            if (connection_ && returner_) {
                returner_(connection_);
            }
        }
        
        // Move-only
        PooledConnection(PooledConnection&& other) noexcept
            : connection_(std::move(other.connection_))
            , returner_(std::move(other.returner_)) {
            other.connection_.reset();
            other.returner_ = nullptr;
        }
        
        PooledConnection& operator=(PooledConnection&& other) noexcept {
            if (this != &other) {
                if (connection_ && returner_) {
                    returner_(connection_);
                }
                connection_ = std::move(other.connection_);
                returner_ = std::move(other.returner_);
                other.connection_.reset();
                other.returner_ = nullptr;
            }
            return *this;
        }
        
        // No copy
        PooledConnection(const PooledConnection&) = delete;
        PooledConnection& operator=(const PooledConnection&) = delete;
        
        DatabaseConnection* operator->() const { return connection_.get(); }
        DatabaseConnection& operator*() const { return *connection_; }
        explicit operator bool() const { return connection_ != nullptr; }
        DatabaseConnection* get() const { return connection_.get(); }
        
    private:
        ConnectionPtr connection_;
        std::function<void(ConnectionPtr)> returner_;
    };
    
    /**
     * @brief Constructor
     * @param io_context Boost.Asio context for timers
     * @param connection_string Database connection string
     * @param config Pool configuration
     */
    ConnectionPool(boost::asio::io_context& io_context,
                   const std::string& connection_string,
                   const PoolConfig& config = PoolConfig())
        : io_context_(io_context)
        , connection_string_(connection_string)
        , config_(config)
        , health_check_timer_(io_context)
        , is_running_(true)
        , total_connections_(0) {
        
        // Pre-create minimum connections
        for (size_t i = 0; i < config_.min_connections; ++i) {
            auto conn = create_connection();
            if (conn && conn->is_open()) {
                available_connections_.push(conn);
            }
        }
        
        // Start health check timer if enabled
        if (config_.enable_health_checks) {
            schedule_health_check();
        }
    }
    
    /**
     * @brief Destructor - closes all connections
     */
    ~ConnectionPool() {
        shutdown();
    }
    
    /**
     * @brief Acquire a connection from the pool
     * @param timeout Maximum time to wait for connection
     * @return Pooled connection that auto-returns on destruction
     */
    PooledConnection acquire(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
        boost::unique_lock<boost::mutex> lock(mutex_);
        
        if (!is_running_) {
            return PooledConnection();
        }
        
        // Wait for available connection or create new one
        if (!condition_.wait_for(lock, boost::chrono::milliseconds(timeout.count()), [this] {
            return !available_connections_.empty() || total_connections_ < config_.max_connections;
        })) {
            return PooledConnection(); // Timeout
        }
        
        ConnectionPtr conn;
        
        // Try to get existing connection
        if (!available_connections_.empty()) {
            conn = available_connections_.front();
            available_connections_.pop();
            
            // Verify connection is still good
            if (!conn->is_open() || !verify_connection(conn)) {
                conn.reset();
            }
        }
        
        // Create new connection if needed
        if (!conn && total_connections_ < config_.max_connections) {
            conn = create_connection();
        }
        
        if (!conn) {
            return PooledConnection();
        }
        
        // Return pooled connection with auto-return function
        return PooledConnection(conn, [this](ConnectionPtr c) {
            return_connection(c);
        });
    }
    
    /**
     * @brief Get current pool statistics
     */
    struct PoolStats {
        size_t total_connections;
        size_t available_connections;
        size_t active_connections;
        size_t failed_connections;
    };
    
    PoolStats get_stats() const {
        boost::lock_guard<boost::mutex> lock(mutex_);
        return {
            total_connections_,
            available_connections_.size(),
            total_connections_ - available_connections_.size(),
            failed_connections_
        };
    }
    
    /**
     * @brief Shutdown the connection pool
     */
    void shutdown() {
        boost::lock_guard<boost::mutex> lock(mutex_);
        is_running_ = false;
        
        // Cancel health check timer
        health_check_timer_.cancel();
        
        // Close all connections
        while (!available_connections_.empty()) {
            available_connections_.pop();
        }
        
        condition_.notify_all();
    }
    
    /**
     * @brief Set minimum pool size
     * @param size Minimum number of connections
     */
    void set_min_size(size_t size) {
        boost::lock_guard<boost::mutex> lock(mutex_);
        config_.min_connections = size;
        
        // Ensure minimum connections
        while (available_connections_.size() + get_active_count() < config_.min_connections 
               && total_connections_ < config_.max_connections) {
            auto conn = create_connection();
            if (conn && conn->is_open()) {
                available_connections_.push(conn);
            }
        }
    }
    
private:
    /**
     * @brief Create a new database connection
     * @return Shared pointer to new connection
     */
    ConnectionPtr create_connection() {
        auto conn = std::make_shared<DatabaseConnection>();
        auto error = conn->open(connection_string_);
        
        if (!error && conn->is_open()) {
            ++total_connections_;
            return conn;
        }
        
        ++failed_connections_;
        return nullptr;
    }
    
    /**
     * @brief Return a connection to the pool
     * @param conn Connection to return
     */
    void return_connection(ConnectionPtr conn) {
        boost::lock_guard<boost::mutex> lock(mutex_);
        
        if (!is_running_) {
            return;
        }
        
        // Check if connection is still healthy
        if (conn && conn->is_open() && verify_connection(conn)) {
            // Check idle timeout
            if (conn->get_idle_time() < config_.idle_timeout) {
                available_connections_.push(conn);
                condition_.notify_one();
                return;
            }
        }
        
        // Connection is bad or too old, close it
        if (conn) {
            conn->close();
            --total_connections_;
        }
        
        // Maintain minimum connections
        if (total_connections_ < config_.min_connections) {
            auto new_conn = create_connection();
            if (new_conn && new_conn->is_open()) {
                available_connections_.push(new_conn);
                condition_.notify_one();
            }
        }
    }
    
    /**
     * @brief Verify a connection is still valid
     * @param conn Connection to verify
     * @return true if connection is healthy
     */
    bool verify_connection(ConnectionPtr conn) {
        if (!conn || !conn->is_open()) {
            return false;
        }
        
        // Use the enhanced health check method
        return conn->check_health();
    }
    
    /**
     * @brief Perform periodic health checks
     */
    void perform_health_check() {
        boost::lock_guard<boost::mutex> lock(mutex_);
        
        if (!is_running_) {
            return;
        }
        
        std::queue<ConnectionPtr> healthy_connections;
        
        // Check all available connections
        while (!available_connections_.empty()) {
            auto conn = available_connections_.front();
            available_connections_.pop();
            
            if (verify_connection(conn) && conn->get_idle_time() < config_.idle_timeout) {
                healthy_connections.push(conn);
            } else {
                conn->close();
                --total_connections_;
            }
        }
        
        // Put healthy connections back
        available_connections_ = std::move(healthy_connections);
        
        // Ensure minimum connections
        while (available_connections_.size() < config_.min_connections 
               && total_connections_ < config_.max_connections) {
            auto conn = create_connection();
            if (conn && conn->is_open()) {
                available_connections_.push(conn);
            }
        }
        
        condition_.notify_all();
    }
    
    /**
     * @brief Schedule next health check
     */
    void schedule_health_check() {
        health_check_timer_.expires_after(config_.health_check_interval);
        health_check_timer_.async_wait([this](boost::system::error_code ec) {
            if (!ec && is_running_) {
                perform_health_check();
                schedule_health_check();
            }
        });
    }
    
    /**
     * @brief Get number of active connections
     */
    size_t get_active_count() const {
        return total_connections_ - available_connections_.size();
    }
    
    boost::asio::io_context& io_context_;
    std::string connection_string_;
    PoolConfig config_;
    
    mutable boost::mutex mutex_;
    boost::condition_variable condition_;
    std::queue<ConnectionPtr> available_connections_;
    boost::asio::steady_timer health_check_timer_;
    
    std::atomic<bool> is_running_;
    std::atomic<size_t> total_connections_;
    std::atomic<size_t> failed_connections_{0};
};

} // namespace liarsdice::database

#endif // LIARSDICE_CONNECTION_POOL_HPP