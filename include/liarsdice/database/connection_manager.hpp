#ifndef LIARSDICE_CONNECTION_MANAGER_HPP
#define LIARSDICE_CONNECTION_MANAGER_HPP

#include <atomic>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/log/trivial.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>
#include <memory>

#include "connection_pool.hpp"
#include "database_config.hpp"

namespace liarsdice::database {

  /**
   * @brief Singleton connection manager with lazy initialization
   *
   * Manages database connection pools and provides thread-safe
   * access to database connections using Boost.Asio for async operations.
   */
  class ConnectionManager : private boost::noncopyable {
  public:
    /**
     * @brief Get singleton instance
     * @return Reference to connection manager
     */
    static ConnectionManager& instance() {
      boost::call_once(init_flag_, &ConnectionManager::initialize);
      return *instance_;
    }

    /**
     * @brief Initialize the connection manager
     * @param config Optional pool configuration
     */
    void configure(const ConnectionPool::PoolConfig& config = ConnectionPool::PoolConfig()) {
      boost::lock_guard<boost::mutex> lock(mutex_);

      if (is_initialized_) {
        throw std::runtime_error("ConnectionManager already initialized");
      }

      pool_config_ = config;

      // Get database configuration
      auto& db_config = DatabaseConfig::instance();
      connection_string_ = db_config.get_connection_string();

      // Create thread pool for async operations
      thread_pool_size_ = db_config.get_thread_pool_size();

      // Start IO context and worker threads
      start_io_context();

      // Create connection pool
      connection_pool_
          = std::make_unique<ConnectionPool>(*io_context_, connection_string_, pool_config_);

      is_initialized_ = true;
    }

    /**
     * @brief Acquire a database connection
     * @param timeout Maximum time to wait
     * @return Pooled connection handle
     */
    ConnectionPool::PooledConnection acquire_connection(std::chrono::milliseconds timeout
                                                        = std::chrono::milliseconds(5000)) {
      if (!is_initialized_) {
        throw std::runtime_error("ConnectionManager not initialized");
      }

      return connection_pool_->acquire(timeout);
    }

    /**
     * @brief Get connection pool statistics
     * @return Pool statistics
     */
    ConnectionPool::PoolStats get_pool_stats() const {
      if (!is_initialized_) {
        return {};
      }

      return connection_pool_->get_stats();
    }

    /**
     * @brief Execute a transaction with automatic rollback on failure and timeout
     * @param func Function to execute within transaction
     * @param timeout_ms Maximum time in milliseconds (default: 30 seconds)
     * @return Error code if transaction failed
     */
    template <typename Func>
    boost::system::error_code execute_transaction(Func func, std::chrono::milliseconds timeout_ms
                                                             = std::chrono::milliseconds(30000)) {
      auto conn = acquire_connection();
      if (!conn) {
        return boost::system::error_code(SQLITE_CANTOPEN, boost::system::generic_category());
      }

      // Set busy timeout on the connection
      sqlite3_busy_timeout(conn->get(), static_cast<int>(timeout_ms.count()));

      // Begin transaction with timeout tracking
      auto start_time = std::chrono::steady_clock::now();
      auto error = conn->begin_transaction();
      if (error) {
        return error;
      }

      try {
        // Check if we've exceeded timeout
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed > timeout_ms) {
          conn->rollback();
          BOOST_LOG_TRIVIAL(warning)
              << "Transaction timed out after "
              << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << "ms";
          return boost::system::error_code(SQLITE_BUSY, boost::system::generic_category());
        }

        // Execute user function
        error = func(*conn);

        if (error) {
          conn->rollback();
          return error;
        }

        // Check timeout again before commit
        elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed > timeout_ms) {
          conn->rollback();
          BOOST_LOG_TRIVIAL(warning)
              << "Transaction timed out during commit after "
              << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << "ms";
          return boost::system::error_code(SQLITE_BUSY, boost::system::generic_category());
        }

        // Commit transaction
        return conn->commit();

      } catch (...) {
        conn->rollback();
        throw;
      }
    }

    /**
     * @brief Execute a function with a connection
     * @param func Function to execute
     * @return Result of function
     */
    template <typename Func> auto with_connection(Func func)
        -> decltype(func(std::declval<DatabaseConnection&>())) {
      auto conn = acquire_connection();
      if (!conn) {
        throw std::runtime_error("Failed to acquire database connection");
      }

      return func(*conn);
    }

    /**
     * @brief Shutdown the connection manager
     */
    void shutdown() {
      boost::lock_guard<boost::mutex> lock(mutex_);

      if (!is_initialized_) {
        return;
      }

      // Shutdown connection pool
      if (connection_pool_) {
        connection_pool_->shutdown();
        connection_pool_.reset();
      }

      // Stop IO context
      stop_io_context();

      is_initialized_ = false;
    }

    /**
     * @brief Check if manager is initialized
     * @return true if initialized
     */
    bool is_initialized() const { return is_initialized_; }

    /**
     * @brief Get the IO context for async operations
     * @return Reference to io_context
     */
    boost::asio::io_context& get_io_context() {
      if (!io_context_) {
        throw std::runtime_error("IO context not initialized");
      }
      return *io_context_;
    }

    /**
     * @brief Set connection pool size limits
     * @param min_size Minimum connections
     * @param max_size Maximum connections
     */
    void set_pool_size(size_t min_size, size_t max_size) {
      boost::lock_guard<boost::mutex> lock(mutex_);

      if (!is_initialized_) {
        pool_config_.min_connections = min_size;
        pool_config_.max_connections = max_size;
      } else if (connection_pool_) {
        connection_pool_->set_min_size(min_size);
        // Note: max_size change requires pool recreation
      }
    }

    /**
     * @brief Enable or disable health checks
     * @param enable true to enable health checks
     */
    void set_health_checks_enabled(bool enable) {
      boost::lock_guard<boost::mutex> lock(mutex_);
      pool_config_.enable_health_checks = enable;
    }

  private:
    /**
     * @brief Private constructor for singleton
     */
    ConnectionManager() : is_initialized_(false), thread_pool_size_(4) {}

    /**
     * @brief Destructor
     */
    ~ConnectionManager() { shutdown(); }

    /**
     * @brief Initialize singleton instance
     */
    static void initialize() {
      // Use a custom deleter to handle private destructor
      instance_ = std::unique_ptr<ConnectionManager>(new ConnectionManager());
    }

    // Friend the unique_ptr's deleter to allow destruction
    friend struct std::default_delete<ConnectionManager>;

    /**
     * @brief Start IO context and worker threads
     */
    void start_io_context() {
      io_context_ = std::make_unique<boost::asio::io_context>();
      work_guard_ = std::make_unique<
          boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
          boost::asio::make_work_guard(*io_context_));

      // Start worker threads
      for (size_t i = 0; i < thread_pool_size_; ++i) {
        worker_threads_.emplace_back([this] { io_context_->run(); });
      }
    }

    /**
     * @brief Stop IO context and join worker threads
     */
    void stop_io_context() {
      if (work_guard_) {
        work_guard_.reset();
      }

      if (io_context_) {
        io_context_->stop();
      }

      // Join all worker threads
      for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
          thread.join();
        }
      }
      worker_threads_.clear();

      io_context_.reset();
    }

    // Singleton members - static definitions in connection_manager.cpp
    static std::unique_ptr<ConnectionManager> instance_;
    static boost::once_flag init_flag_;

    // Instance members
    mutable boost::mutex mutex_;
    std::atomic<bool> is_initialized_;

    // Connection management
    std::unique_ptr<ConnectionPool> connection_pool_;
    ConnectionPool::PoolConfig pool_config_;
    std::string connection_string_;

    // Async IO
    std::unique_ptr<boost::asio::io_context> io_context_;
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>
        work_guard_;
    std::vector<boost::thread> worker_threads_;
    size_t thread_pool_size_;
  };

  // Static member declarations - definitions moved to connection_manager.cpp

}  // namespace liarsdice::database

#endif  // LIARSDICE_CONNECTION_MANAGER_HPP