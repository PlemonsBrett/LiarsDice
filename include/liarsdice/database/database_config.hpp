#ifndef LIARSDICE_DATABASE_CONFIG_HPP
#define LIARSDICE_DATABASE_CONFIG_HPP

#include <boost/filesystem.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <optional>
#include <string>

namespace liarsdice::database {

  namespace fs = boost::filesystem;

  /**
   * @brief Database configuration and file management
   *
   * Manages database file paths, initialization, and configuration
   * using Boost.Filesystem for cross-platform path handling.
   */
  class DatabaseConfig {
  public:
    /**
     * @brief Get singleton instance
     */
    static DatabaseConfig& instance() {
      static DatabaseConfig config;
      return config;
    }

    /**
     * @brief Set the database directory path
     * @param path Directory where database files will be stored
     * @throws std::runtime_error if path is invalid or cannot be created
     */
    void set_database_directory(const fs::path& path) {
      boost::lock_guard<boost::mutex> lock(mutex_);

      // Create directory if it doesn't exist
      if (!fs::exists(path)) {
        boost::system::error_code ec;
        if (!fs::create_directories(path, ec)) {
          throw std::runtime_error("Failed to create database directory: " + ec.message());
        }
      }

      if (!fs::is_directory(path)) {
        throw std::runtime_error("Database path is not a directory: " + path.string());
      }

      database_dir_ = fs::canonical(path);
    }

    /**
     * @brief Get the database directory path
     * @return Path to database directory
     */
    fs::path get_database_directory() const {
      boost::lock_guard<boost::mutex> lock(mutex_);
      if (!database_dir_) {
        // Default to user's home directory/.liarsdice/db
        fs::path home = fs::path(std::getenv("HOME") ? std::getenv("HOME") : ".");
        return home / ".liarsdice" / "db";
      }
      return *database_dir_;
    }

    /**
     * @brief Get the main database file path
     * @return Full path to the main database file
     */
    fs::path get_database_file_path() const { return get_database_directory() / "liarsdice.db"; }

    /**
     * @brief Get the backup database file path
     * @param backup_id Optional backup identifier
     * @return Full path to the backup database file
     */
    fs::path get_backup_file_path(const std::string& backup_id = "") const {
      if (backup_id.empty()) {
        return get_database_directory() / "liarsdice_backup.db";
      }
      return get_database_directory() / ("liarsdice_backup_" + backup_id + ".db");
    }

    /**
     * @brief Get SQLite connection string with options
     * @return Connection string with thread safety and performance options
     */
    std::string get_connection_string() const {
      std::string conn_str = get_database_file_path().string();

      // Add SQLite URI parameters for thread safety and performance
      conn_str = "file:" + conn_str + "?mode=rwc" +  // Read-write, create if not exists
                 "&cache=shared" +                   // Shared cache mode
                 "&psow=1" +                         // Powersafe overwrite
                 "&nolock=0";                        // Use locking (thread safe)

      return conn_str;
    }

    /**
     * @brief Check if database file exists
     * @return true if database file exists
     */
    bool database_exists() const { return fs::exists(get_database_file_path()); }

    /**
     * @brief Get database file size
     * @return Size in bytes, or 0 if file doesn't exist
     */
    std::uintmax_t get_database_size() const {
      if (database_exists()) {
        boost::system::error_code ec;
        auto size = fs::file_size(get_database_file_path(), ec);
        return ec ? 0 : size;
      }
      return 0;
    }

    /**
     * @brief Enable SQLite extensions
     * @return true if extensions are enabled
     */
    bool extensions_enabled() const { return extensions_enabled_; }
    void set_extensions_enabled(bool enabled) {
      boost::lock_guard<boost::mutex> lock(mutex_);
      extensions_enabled_ = enabled;
    }

    /**
     * @brief Get thread pool size for database operations
     */
    unsigned int get_thread_pool_size() const {
      boost::lock_guard<boost::mutex> lock(mutex_);
      return thread_pool_size_;
    }

    void set_thread_pool_size(unsigned int size) {
      boost::lock_guard<boost::mutex> lock(mutex_);
      thread_pool_size_ = size > 0 ? size : 1;
    }

  private:
    DatabaseConfig() = default;
    DatabaseConfig(const DatabaseConfig&) = delete;
    DatabaseConfig& operator=(const DatabaseConfig&) = delete;

    mutable boost::mutex mutex_;
    std::optional<fs::path> database_dir_;
    bool extensions_enabled_ = true;
    unsigned int thread_pool_size_ = 4;
  };

}  // namespace liarsdice::database

#endif  // LIARSDICE_DATABASE_CONFIG_HPP