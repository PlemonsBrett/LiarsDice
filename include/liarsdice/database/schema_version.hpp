#ifndef LIARSDICE_SCHEMA_VERSION_HPP
#define LIARSDICE_SCHEMA_VERSION_HPP

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <string>

namespace liarsdice::database {

  /**
   * @brief Represents a database schema version
   */
  struct SchemaVersion {
    std::string id;           // Unique migration ID (UUID)
    int version;              // Sequential version number
    std::string description;  // Human-readable description
    std::string checksum;     // SHA256 of the migration SQL
    std::chrono::system_clock::time_point applied_at;
    bool is_applied;

    SchemaVersion() : version(0), is_applied(false) {}

    SchemaVersion(int ver, const std::string& desc)
        : version(ver), description(desc), is_applied(false) {
      // Generate UUID for migration ID
      boost::uuids::random_generator gen;
      id = boost::uuids::to_string(gen());
    }

    // Compare versions for ordering
    bool operator<(const SchemaVersion& other) const { return version < other.version; }

    bool operator==(const SchemaVersion& other) const {
      return version == other.version && checksum == other.checksum;
    }
  };

}  // namespace liarsdice::database

#endif  // LIARSDICE_SCHEMA_VERSION_HPP