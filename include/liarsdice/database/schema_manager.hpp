#ifndef LIARSDICE_SCHEMA_MANAGER_HPP
#define LIARSDICE_SCHEMA_MANAGER_HPP

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/thread/mutex.hpp>
#include <memory>
#include <vector>

#include "database_error.hpp"
#include "database_manager.hpp"
#include "migration.hpp"

namespace liarsdice::database {

  /**
   * @brief Manages database schema versioning and migrations
   *
   * This class handles:
   * - Schema version tracking
   * - Migration execution and rollback
   * - Automatic schema updates
   * - Migration validation
   */
  class SchemaManager {
  public:
    explicit SchemaManager(DatabaseManager& db) : db_(db) { initialize_schema_table(); }

    /**
     * @brief Add a migration to the manager
     */
    void add_migration(std::unique_ptr<Migration> migration) {
      boost::lock_guard<boost::mutex> lock(mutex_);
      migrations_.push_back(std::move(migration));
      std::sort(migrations_.begin(), migrations_.end(), [](const auto& a, const auto& b) {
        return a->get_version().version < b->get_version().version;
      });
    }

    /**
     * @brief Load migrations from a directory
     * @param dir Directory containing migration files
     *
     * Expected file naming convention:
     * - V001__description.up.sql   (upgrade script)
     * - V001__description.down.sql (downgrade script, optional)
     */
    DatabaseResult<int> load_migrations_from_directory(const boost::filesystem::path& dir);

    /**
     * @brief Get current schema version
     */
    DatabaseResult<int> get_current_version();

    /**
     * @brief Get all applied migrations
     */
    DatabaseResult<std::vector<SchemaVersion>> get_applied_migrations();

    /**
     * @brief Check if a specific version is applied
     */
    DatabaseResult<bool> is_version_applied(int version);

    /**
     * @brief Migrate to a specific version
     * @param target_version Target version (-1 for latest)
     */
    DatabaseResult<void> migrate_to(int target_version = -1);

    /**
     * @brief Rollback to a specific version
     * @param target_version Target version to rollback to
     */
    DatabaseResult<void> rollback_to(int target_version);

    /**
     * @brief Get pending migrations
     */
    DatabaseResult<std::vector<Migration*>> get_pending_migrations();

    /**
     * @brief Validate all migrations
     */
    DatabaseResult<void> validate_migrations();

    /**
     * @brief Clean migration history (remove orphaned entries)
     */
    DatabaseResult<void> clean_migration_history();

    /**
     * @brief Create a baseline version
     * @param version Version to mark as baseline
     */
    DatabaseResult<void> baseline(int version, const std::string& description);

  private:
    DatabaseManager& db_;
    std::vector<std::unique_ptr<Migration>> migrations_;
    mutable boost::mutex mutex_;

    /**
     * @brief Initialize the schema version table
     */
    void initialize_schema_table();

    /**
     * @brief Record a migration as applied
     */
    DatabaseResult<void> record_migration(const SchemaVersion& version);

    /**
     * @brief Remove a migration record
     */
    DatabaseResult<void> remove_migration_record(int version);

    /**
     * @brief Execute a single migration
     */
    DatabaseResult<void> execute_migration(Migration* migration);

    /**
     * @brief Execute a single rollback
     */
    DatabaseResult<void> execute_rollback(Migration* migration);

    /**
     * @brief Parse migration filename
     */
    std::pair<int, std::string> parse_migration_filename(const std::string& filename);
  };

}  // namespace liarsdice::database

#endif  // LIARSDICE_SCHEMA_MANAGER_HPP