#include <liarsdice/database/migration.hpp>
#include <liarsdice/database/database_manager.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/uuid/detail/sha1.hpp>
#include <sstream>
#include <fstream>

namespace liarsdice::database {

void Migration::calculate_checksum() {
    // Calculate SHA1 checksum of the migration content
    boost::uuids::detail::sha1 sha;
    
    if (!up_sql_.empty()) {
        sha.process_bytes(up_sql_.data(), up_sql_.size());
    }
    
    if (!down_sql_.empty()) {
        sha.process_bytes(down_sql_.data(), down_sql_.size());
    }
    
    // If using functions, use version and description for checksum
    if (up_function_) {
        std::string func_data = std::to_string(version_.version) + version_.description;
        sha.process_bytes(func_data.data(), func_data.size());
    }
    
    boost::uuids::detail::sha1::digest_type digest;
    sha.get_digest(digest);
    
    // Convert to hex string
    std::ostringstream oss;
    for (int i = 0; i < 5; ++i) {
        oss << std::hex << std::setfill('0') << std::setw(8) << digest[i];
    }
    
    version_.checksum = oss.str();
}

void Migration::apply(DatabaseManager& db) {
    if (up_function_) {
        up_function_(db);
    } else if (!up_sql_.empty()) {
        auto result = db.execute(up_sql_);
        if (!result) {
            throw std::runtime_error("Migration failed: " + result.error().full_message());
        }
    } else {
        throw std::runtime_error("Migration has no upgrade path");
    }
}

void Migration::rollback(DatabaseManager& db) {
    if (!is_reversible()) {
        throw std::runtime_error("Migration is not reversible");
    }
    
    if (down_function_) {
        down_function_(db);
    } else if (!down_sql_.empty()) {
        auto result = db.execute(down_sql_);
        if (!result) {
            throw std::runtime_error("Rollback failed: " + result.error().full_message());
        }
    }
}

// FileMigration implementation
FileMigration::FileMigration(int version, const boost::filesystem::path& up_file,
                             const boost::filesystem::path& down_file)
    : Migration(version, up_file.stem().string(), 
                read_file(up_file),
                down_file.empty() ? "" : read_file(down_file)) {
}

std::string FileMigration::read_file(const boost::filesystem::path& path) {
    if (!boost::filesystem::exists(path)) {
        throw std::runtime_error("Migration file not found: " + path.string());
    }
    
    std::ifstream file(path.string());
    if (!file) {
        throw std::runtime_error("Failed to open migration file: " + path.string());
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace liarsdice::database