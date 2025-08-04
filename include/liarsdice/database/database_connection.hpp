#ifndef LIARSDICE_DATABASE_CONNECTION_HPP
#define LIARSDICE_DATABASE_CONNECTION_HPP

#include <sqlite3.h>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/system/error_code.hpp>
#include <memory>
#include <string>
#include <chrono>
#include <functional>

namespace liarsdice::database {

/**
 * @brief RAII wrapper for SQLite database connection
 * 
 * Manages a single SQLite database connection with automatic
 * resource cleanup and thread-safe operations.
 */
class DatabaseConnection : private boost::noncopyable {
public:
    using ConnectionPtr = std::unique_ptr<sqlite3, std::function<void(sqlite3*)>>;
    
    /**
     * @brief Connection state
     */
    enum class State {
        Disconnected,
        Connected,
        Error
    };
    
    /**
     * @brief Constructor - creates a closed connection
     */
    DatabaseConnection() 
        : state_(State::Disconnected)
        , last_activity_(std::chrono::steady_clock::now()) {}
    
    /**
     * @brief Destructor - ensures connection is closed
     */
    ~DatabaseConnection() {
        close();
    }
    
    /**
     * @brief Open a database connection
     * @param path Database file path or URI
     * @param flags SQLite open flags
     * @return Error code if failed
     */
    boost::system::error_code open(const std::string& path, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) {
        boost::lock_guard<boost::mutex> lock(mutex_);
        
        if (state_ == State::Connected) {
            return boost::system::error_code();
        }
        
        // Check if this is a URI (starts with "file:")
        if (path.substr(0, 5) == "file:") {
            flags |= SQLITE_OPEN_URI;
        }
        
        sqlite3* db_raw = nullptr;
        int result = sqlite3_open_v2(path.c_str(), &db_raw, flags, nullptr);
        
        if (result != SQLITE_OK) {
            if (db_raw) {
                sqlite3_close(db_raw);
            }
            state_ = State::Error;
            return boost::system::error_code(result, boost::system::generic_category());
        }
        
        // Set up connection with custom deleter
        connection_ = ConnectionPtr(db_raw, [](sqlite3* db) {
            if (db) {
                sqlite3_close(db);
            }
        });
        
        state_ = State::Connected;
        connection_string_ = path;
        update_activity();
        
        // Configure connection settings
        configure_connection();
        
        return boost::system::error_code();
    }
    
    /**
     * @brief Close the database connection
     */
    void close() {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (connection_) {
            connection_.reset();
            state_ = State::Disconnected;
            connection_string_.clear();
        }
    }
    
    /**
     * @brief Check if connection is open
     * @return true if connected
     */
    bool is_open() const {
        boost::lock_guard<boost::mutex> lock(mutex_);
        return state_ == State::Connected && connection_ != nullptr;
    }
    
    /**
     * @brief Get the current connection state
     * @return Connection state
     */
    State get_state() const {
        boost::lock_guard<boost::mutex> lock(mutex_);
        return state_;
    }
    
    /**
     * @brief Get the raw SQLite connection handle
     * @return SQLite3 pointer (may be null)
     */
    sqlite3* get() const {
        boost::lock_guard<boost::mutex> lock(mutex_);
        update_activity();
        return connection_ ? connection_.get() : nullptr;
    }
    
    /**
     * @brief Execute a simple SQL statement
     * @param sql SQL statement to execute
     * @return Error code if failed
     */
    boost::system::error_code execute(const std::string& sql) {
        boost::lock_guard<boost::mutex> lock(mutex_);
        
        if (!connection_) {
            return boost::system::error_code(SQLITE_MISUSE, boost::system::generic_category());
        }
        
        update_activity();
        
        char* error_msg = nullptr;
        int result = sqlite3_exec(connection_.get(), sql.c_str(), nullptr, nullptr, &error_msg);
        
        if (result != SQLITE_OK) {
            std::string error_str = error_msg ? error_msg : "Unknown error";
            sqlite3_free(error_msg);
            last_error_ = error_str;
            return boost::system::error_code(result, boost::system::generic_category());
        }
        
        return boost::system::error_code();
    }
    
    /**
     * @brief Begin a transaction
     * @return Error code if failed
     */
    boost::system::error_code begin_transaction() {
        return execute("BEGIN TRANSACTION");
    }
    
    /**
     * @brief Commit a transaction
     * @return Error code if failed
     */
    boost::system::error_code commit() {
        return execute("COMMIT");
    }
    
    /**
     * @brief Rollback a transaction
     * @return Error code if failed
     */
    boost::system::error_code rollback() {
        return execute("ROLLBACK");
    }
    
    /**
     * @brief Get the last error message
     * @return Error message string
     */
    std::string get_last_error() const {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (connection_) {
            return sqlite3_errmsg(connection_.get());
        }
        return last_error_;
    }
    
    /**
     * @brief Get the last insert row ID
     * @return Row ID or 0 if no insert
     */
    sqlite3_int64 get_last_insert_rowid() const {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (connection_) {
            return sqlite3_last_insert_rowid(connection_.get());
        }
        return 0;
    }
    
    /**
     * @brief Get the number of rows changed by last statement
     * @return Number of changed rows
     */
    int get_changes() const {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (connection_) {
            return sqlite3_changes(connection_.get());
        }
        return 0;
    }
    
    /**
     * @brief Get time since last activity
     * @return Duration since last use
     */
    std::chrono::steady_clock::duration get_idle_time() const {
        boost::lock_guard<boost::mutex> lock(mutex_);
        return std::chrono::steady_clock::now() - last_activity_;
    }
    
    /**
     * @brief Get the connection string
     * @return Connection string used to open database
     */
    std::string get_connection_string() const {
        boost::lock_guard<boost::mutex> lock(mutex_);
        return connection_string_;
    }
    
    /**
     * @brief Set busy timeout
     * @param ms Timeout in milliseconds
     */
    void set_busy_timeout(int ms) {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (connection_) {
            sqlite3_busy_timeout(connection_.get(), ms);
        }
    }
    
private:
    /**
     * @brief Configure connection settings (must be called with mutex locked)
     */
    void configure_connection() {
        if (!connection_) return;
        
        // Execute pragmas directly without locking again
        char* error_msg = nullptr;
        
        // Set pragmas for performance and safety
        const char* pragmas[] = {
            "PRAGMA journal_mode=WAL",
            "PRAGMA synchronous=NORMAL",
            "PRAGMA foreign_keys=ON",
            "PRAGMA cache_size=-64000", // 64MB cache
            "PRAGMA temp_store=MEMORY",
            "PRAGMA mmap_size=268435456" // 256MB mmap
        };
        
        for (const char* pragma : pragmas) {
            int result = sqlite3_exec(connection_.get(), pragma, nullptr, nullptr, &error_msg);
            if (result != SQLITE_OK && error_msg) {
                sqlite3_free(error_msg);
            }
        }
        
        // Set busy timeout directly
        sqlite3_busy_timeout(connection_.get(), 5000); // 5 seconds
    }
    
    /**
     * @brief Update last activity timestamp
     */
    void update_activity() const {
        last_activity_ = std::chrono::steady_clock::now();
    }
    
    mutable boost::mutex mutex_;
    ConnectionPtr connection_;
    State state_;
    std::string connection_string_;
    std::string last_error_;
    mutable std::chrono::steady_clock::time_point last_activity_;
};

} // namespace liarsdice::database

#endif // LIARSDICE_DATABASE_CONNECTION_HPP