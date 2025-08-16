#ifndef LIARSDICE_DATABASE_MANAGER_HPP
#define LIARSDICE_DATABASE_MANAGER_HPP

#include "database_connection.hpp"
#include "connection_manager.hpp"
#include "prepared_statement.hpp"
#include "database_error.hpp"
#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/scope_exit.hpp>
#include <boost/log/trivial.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <list>

namespace liarsdice::database {

/**
 * @brief Core database manager with transaction support and prepared statement caching
 */
class DatabaseManager : private boost::noncopyable {
public:
    using QueryCallback = std::function<void(const PreparedStatement&)>;
    using TransactionFunc = std::function<DatabaseResult<void>(DatabaseManager&)>;
    
    /**
     * @brief Constructor
     */
    DatabaseManager() : in_transaction_(false) {
        BOOST_LOG_TRIVIAL(debug) << "DatabaseManager created";
    }
    
    /**
     * @brief Destructor
     */
    ~DatabaseManager() {
        if (in_transaction_) {
            try {
                rollback_transaction();
            } catch (...) {
                // Suppress exceptions in destructor
            }
        }
    }
    
    /**
     * @brief Execute a query with automatic connection management
     * @param sql SQL query to execute
     * @return Result or error
     */
    DatabaseResult<void> execute(const std::string& sql) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        try {
            auto& conn_mgr = ConnectionManager::instance();
            auto conn = conn_mgr.acquire_connection();
            
            if (!conn) {
                BOOST_LOG_TRIVIAL(error) << "Failed to acquire database connection";
                return DatabaseError(DatabaseErrorType::ConnectionFailed, 
                                   "Failed to acquire database connection");
            }
            
            auto error = conn->execute(sql);
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - start_time);
            
            if (error) {
                BOOST_LOG_TRIVIAL(error) << "Query failed: " << sql 
                                        << " Error: " << error.message();
                return DatabaseError(DatabaseErrorType::QueryFailed, 
                                   "Query execution failed", error);
            }
            
            BOOST_LOG_TRIVIAL(trace) << "Query executed in " << duration.count() 
                                    << "μs: " << sql;
            
            return DatabaseResult<void>();
            
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Exception in execute: " << e.what();
            return DatabaseError(DatabaseErrorType::InternalError, e.what());
        }
    }
    
    /**
     * @brief Prepare a statement with caching and LRU eviction
     * @param sql SQL statement to prepare
     * @return Prepared statement or error
     */
    DatabaseResult<std::shared_ptr<PreparedStatement>> prepare(const std::string& sql) {
        boost::lock_guard<boost::mutex> lock(cache_mutex_);
        
        // Check cache first
        auto it = statement_cache_.find(sql);
        if (it != statement_cache_.end()) {
            // Update LRU position
            auto lru_it = cache_lru_map_[sql];
            cache_lru_list_.erase(lru_it);
            cache_lru_list_.push_front(sql);
            cache_lru_map_[sql] = cache_lru_list_.begin();
            
            // Reset and return cached statement
            it->second->reset();
            it->second->clear_bindings();
            BOOST_LOG_TRIVIAL(trace) << "Using cached prepared statement";
            return it->second;
        }
        
        // Check cache size limits before adding new statement
        if (statement_cache_.size() >= MAX_CACHED_STATEMENTS) {
            // Evict least recently used statement
            std::string lru_key = cache_lru_list_.back();
            cache_lru_list_.pop_back();
            cache_lru_map_.erase(lru_key);
            statement_cache_.erase(lru_key);
            BOOST_LOG_TRIVIAL(trace) << "Evicted LRU statement from cache";
        }
        
        // Check estimated memory usage
        CacheStats stats = get_cache_stats();
        if (stats.cache_memory_estimate > MAX_CACHE_MEMORY_MB * 1024 * 1024) {
            // Clear half of the cache if memory limit exceeded
            size_t to_remove = statement_cache_.size() / 2;
            for (size_t i = 0; i < to_remove && !cache_lru_list_.empty(); ++i) {
                std::string lru_key = cache_lru_list_.back();
                cache_lru_list_.pop_back();
                cache_lru_map_.erase(lru_key);
                statement_cache_.erase(lru_key);
            }
            BOOST_LOG_TRIVIAL(warning) << "Cache memory limit exceeded, cleared " 
                                       << to_remove << " statements";
        }
        
        // Prepare new statement
        try {
            auto& conn_mgr = ConnectionManager::instance();
            auto conn = conn_mgr.acquire_connection();
            
            if (!conn) {
                return DatabaseError(DatabaseErrorType::ConnectionFailed, 
                                   "Failed to acquire database connection");
            }
            
            sqlite3_stmt* stmt_raw = nullptr;
            int result = sqlite3_prepare_v2(conn->get(), sql.c_str(), -1, &stmt_raw, nullptr);
            
            if (result != SQLITE_OK) {
                std::string error_msg = conn->get_last_error();
                BOOST_LOG_TRIVIAL(error) << "Failed to prepare statement: " << sql 
                                        << " Error: " << error_msg;
                return DatabaseError(DatabaseErrorType::PreparedStatementFailed, 
                                   "Failed to prepare statement: " + error_msg);
            }
            
            auto stmt = std::make_shared<PreparedStatement>(stmt_raw, sql);
            
            // Cache the statement with LRU tracking
            statement_cache_[sql] = stmt;
            cache_lru_list_.push_front(sql);
            cache_lru_map_[sql] = cache_lru_list_.begin();
            
            BOOST_LOG_TRIVIAL(debug) << "Prepared and cached statement: " << sql 
                                    << " (cache size: " << statement_cache_.size() << ")";
            
            return stmt;
            
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Exception in prepare: " << e.what();
            return DatabaseError(DatabaseErrorType::InternalError, e.what());
        }
    }
    
    /**
     * @brief Execute a prepared statement
     * @param stmt Prepared statement
     * @param callback Optional callback for each row
     * @return Result or error
     */
    DatabaseResult<int> execute_prepared(std::shared_ptr<PreparedStatement> stmt, 
                                        QueryCallback callback = nullptr) {
        if (!stmt || !*stmt) {
            return DatabaseError(DatabaseErrorType::InvalidParameter, 
                               "Invalid prepared statement");
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        int rows_affected = 0;
        
        try {
            int result;
            while ((result = stmt->step()) == SQLITE_ROW) {
                if (callback) {
                    callback(*stmt);
                }
                rows_affected++;
            }
            
            if (result != SQLITE_DONE) {
                auto& conn_mgr = ConnectionManager::instance();
                auto conn = conn_mgr.acquire_connection();
                std::string error_msg = conn ? conn->get_last_error() : "Unknown error";
                
                BOOST_LOG_TRIVIAL(error) << "Statement execution failed: " << error_msg;
                return DatabaseError(DatabaseErrorType::QueryFailed, 
                                   "Statement execution failed: " + error_msg);
            }
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - start_time);
            
            BOOST_LOG_TRIVIAL(trace) << "Prepared statement executed in " 
                                    << duration.count() << "μs, " 
                                    << rows_affected << " rows affected";
            
            return rows_affected;
            
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Exception in execute_prepared: " << e.what();
            return DatabaseError(DatabaseErrorType::InternalError, e.what());
        }
    }
    
    /**
     * @brief Begin a transaction
     * @return Result or error
     */
    DatabaseResult<void> begin_transaction() {
        if (in_transaction_) {
            return DatabaseError(DatabaseErrorType::TransactionFailed, 
                               "Transaction already in progress");
        }
        
        auto& conn_mgr = ConnectionManager::instance();
        
        // Acquire connection for transaction
        transaction_conn_ = conn_mgr.acquire_connection();
        if (!transaction_conn_) {
            return DatabaseError(DatabaseErrorType::ConnectionFailed, 
                               "Failed to acquire connection for transaction");
        }
        
        auto error = transaction_conn_->begin_transaction();
        if (error) {
            transaction_conn_ = ConnectionPool::PooledConnection();
            return DatabaseError(DatabaseErrorType::TransactionFailed, 
                               "Failed to begin transaction", error);
        }
        
        in_transaction_ = true;
        BOOST_LOG_TRIVIAL(debug) << "Transaction started";
        
        return DatabaseResult<void>();
    }
    
    /**
     * @brief Commit the current transaction
     * @return Result or error
     */
    DatabaseResult<void> commit_transaction() {
        if (!in_transaction_ || !transaction_conn_) {
            return DatabaseError(DatabaseErrorType::TransactionFailed, 
                               "No transaction in progress");
        }
        
        auto error = transaction_conn_->commit();
        
        // Clean up transaction state
        transaction_conn_ = ConnectionPool::PooledConnection();
        in_transaction_ = false;
        
        if (error) {
            BOOST_LOG_TRIVIAL(error) << "Failed to commit transaction";
            return DatabaseError(DatabaseErrorType::TransactionFailed, 
                               "Failed to commit transaction", error);
        }
        
        BOOST_LOG_TRIVIAL(debug) << "Transaction committed";
        return DatabaseResult<void>();
    }
    
    /**
     * @brief Rollback the current transaction
     * @return Result or error
     */
    DatabaseResult<void> rollback_transaction() {
        if (!in_transaction_ || !transaction_conn_) {
            return DatabaseError(DatabaseErrorType::TransactionFailed, 
                               "No transaction in progress");
        }
        
        auto error = transaction_conn_->rollback();
        
        // Clean up transaction state
        transaction_conn_ = ConnectionPool::PooledConnection();
        in_transaction_ = false;
        
        if (error) {
            BOOST_LOG_TRIVIAL(error) << "Failed to rollback transaction";
            return DatabaseError(DatabaseErrorType::TransactionFailed, 
                               "Failed to rollback transaction", error);
        }
        
        BOOST_LOG_TRIVIAL(debug) << "Transaction rolled back";
        return DatabaseResult<void>();
    }
    
    /**
     * @brief Execute a function within a transaction with automatic rollback on failure
     * @param func Function to execute
     * @return Result or error
     */
    DatabaseResult<void> with_transaction(TransactionFunc func) {
        // Begin transaction
        auto begin_result = begin_transaction();
        if (!begin_result) {
            return begin_result;
        }
        
        // Use scope_exit for automatic rollback
        bool should_rollback = true;
        BOOST_SCOPE_EXIT(&should_rollback, this_) {
            if (should_rollback && this_->in_transaction_) {
                this_->rollback_transaction();
            }
        } BOOST_SCOPE_EXIT_END
        
        try {
            // Execute the function
            auto result = func(*this);
            
            if (result) {
                // Success - commit
                auto commit_result = commit_transaction();
                if (commit_result) {
                    should_rollback = false;
                    return DatabaseResult<void>();
                }
                return commit_result;
            } else {
                // Function returned error
                BOOST_LOG_TRIVIAL(warning) << "Transaction function failed: " 
                                          << result.error().full_message();
                return result;
            }
            
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Exception in transaction: " << e.what();
            return DatabaseError(DatabaseErrorType::TransactionFailed, 
                               "Exception in transaction: " + std::string(e.what()));
        }
    }
    
    /**
     * @brief Clear the prepared statement cache
     */
    void clear_statement_cache() {
        boost::lock_guard<boost::mutex> lock(cache_mutex_);
        statement_cache_.clear();
        BOOST_LOG_TRIVIAL(debug) << "Prepared statement cache cleared";
    }
    
    /**
     * @brief Get cache statistics
     */
    struct CacheStats {
        size_t cached_statements;
        size_t cache_memory_estimate;
    };
    
    CacheStats get_cache_stats() const {
        boost::lock_guard<boost::mutex> lock(cache_mutex_);
        CacheStats stats;
        stats.cached_statements = statement_cache_.size();
        stats.cache_memory_estimate = stats.cached_statements * 
                                     (sizeof(PreparedStatement) + 100); // Rough estimate
        return stats;
    }
    
    /**
     * @brief Check if currently in a transaction
     */
    bool in_transaction() const { return in_transaction_; }
    
private:
    // Cache size limits
    static constexpr size_t MAX_CACHED_STATEMENTS = 100;
    static constexpr size_t MAX_CACHE_MEMORY_MB = 10;
    
    // Statement cache with LRU tracking
    mutable boost::mutex cache_mutex_;
    boost::unordered_map<std::string, std::shared_ptr<PreparedStatement>> statement_cache_;
    std::list<std::string> cache_lru_list_;  // LRU tracking
    boost::unordered_map<std::string, std::list<std::string>::iterator> cache_lru_map_;
    
    // Transaction management
    bool in_transaction_;
    ConnectionPool::PooledConnection transaction_conn_;
};

} // namespace liarsdice::database

#endif // LIARSDICE_DATABASE_MANAGER_HPP