#ifndef LIARSDICE_DATABASE_BACKUP_MANAGER_HPP
#define LIARSDICE_DATABASE_BACKUP_MANAGER_HPP

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <chrono>
#include <liarsdice/database/database_error.hpp>
#include <liarsdice/database/database_manager.hpp>
#include <string>
#include <vector>

namespace liarsdice::database {

  /**
   * @brief Backup retention policy configuration
   */
  struct RetentionPolicy {
    // Keep backups for these periods
    int daily_keep_days = 7;      // Keep daily backups for 7 days
    int weekly_keep_weeks = 4;    // Keep weekly backups for 4 weeks
    int monthly_keep_months = 6;  // Keep monthly backups for 6 months
    int yearly_keep_years = 2;    // Keep yearly backups for 2 years

    // Size limits
    size_t max_backup_size_mb = 1000;  // Maximum backup size in MB
    size_t max_total_size_gb = 10;     // Maximum total backup directory size in GB

    // File naming patterns
    std::string daily_pattern = "backup_daily_%Y%m%d_%H%M%S.db";
    std::string weekly_pattern = "backup_weekly_%Y_W%W.db";
    std::string monthly_pattern = "backup_monthly_%Y_%m.db";
    std::string yearly_pattern = "backup_yearly_%Y.db";

    // Compression
    bool compress_backups = true;
    std::string compression_suffix = ".gz";
  };

  /**
   * @brief Backup information structure
   */
  struct BackupInfo {
    boost::filesystem::path file_path;
    std::chrono::system_clock::time_point created_at;
    size_t file_size;
    std::string backup_type;  // daily, weekly, monthly, yearly, manual
    bool is_compressed;
    std::string checksum;  // SHA256 checksum for integrity
  };

  /**
   * @brief Database backup and recovery manager using boost::filesystem
   */
  class BackupManager {
  public:
    explicit BackupManager(DatabaseManager& db, const boost::filesystem::path& backup_dir);

    /**
     * @brief Set retention policy for backups
     */
    void set_retention_policy(const RetentionPolicy& policy);

    /**
     * @brief Create a manual backup with optional name
     */
    DatabaseResult<BackupInfo> create_backup(const std::string& backup_name = "");

    /**
     * @brief Create scheduled backup (daily/weekly/monthly/yearly)
     */
    DatabaseResult<BackupInfo> create_scheduled_backup(const std::string& backup_type);

    /**
     * @brief Restore database from backup file
     */
    DatabaseResult<void> restore_from_backup(const boost::filesystem::path& backup_file);

    /**
     * @brief List all available backups
     */
    DatabaseResult<std::vector<BackupInfo>> list_backups();

    /**
     * @brief Apply retention policy (cleanup old backups)
     */
    DatabaseResult<void> apply_retention_policy();

    /**
     * @brief Verify backup integrity using checksums
     */
    DatabaseResult<bool> verify_backup(const boost::filesystem::path& backup_file);

    /**
     * @brief Get backup directory usage statistics
     */
    DatabaseResult<std::map<std::string, size_t>> get_storage_statistics();

    /**
     * @brief Compress existing backup file
     */
    DatabaseResult<boost::filesystem::path> compress_backup(
        const boost::filesystem::path& backup_file);

    /**
     * @brief Decompress backup file
     */
    DatabaseResult<boost::filesystem::path> decompress_backup(
        const boost::filesystem::path& compressed_file);

    /**
     * @brief Create backup with custom SQL dump
     */
    DatabaseResult<BackupInfo> create_sql_dump(const std::string& dump_name = "");

    /**
     * @brief Import from SQL dump file
     */
    DatabaseResult<void> import_sql_dump(const boost::filesystem::path& dump_file);

    /**
     * @brief Schedule automatic backups (sets up recurring backup creation)
     */
    DatabaseResult<void> schedule_automatic_backups(bool enable = true);

  private:
    DatabaseManager& db_;
    boost::filesystem::path backup_dir_;
    RetentionPolicy retention_policy_;

    // Helper methods
    std::string generate_backup_filename(const std::string& backup_type);
    DatabaseResult<void> copy_database_file(const boost::filesystem::path& dest);
    std::string calculate_file_checksum(const boost::filesystem::path& file_path);
    BackupInfo create_backup_info(const boost::filesystem::path& file_path,
                                  const std::string& backup_type);
    std::vector<BackupInfo> scan_backup_directory();
    bool should_keep_backup(const BackupInfo& backup);
    DatabaseResult<void> cleanup_old_backups(const std::vector<BackupInfo>& backups);
    size_t get_directory_size(const boost::filesystem::path& directory);
    std::string format_backup_filename(const std::string& pattern,
                                       const std::chrono::system_clock::time_point& time_point);
    DatabaseResult<void> ensure_backup_directory();
    bool is_backup_file(const boost::filesystem::path& file_path);
    std::string extract_backup_type(const std::string& filename);
  };

}  // namespace liarsdice::database

#endif  // LIARSDICE_DATABASE_BACKUP_MANAGER_HPP