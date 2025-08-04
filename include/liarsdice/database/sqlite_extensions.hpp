#ifndef LIARSDICE_SQLITE_EXTENSIONS_HPP
#define LIARSDICE_SQLITE_EXTENSIONS_HPP

// Note: Boost.DLL functionality simplified for now
// #include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/mutex.hpp>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace liarsdice::database {

namespace fs = boost::filesystem;

/**
 * @brief SQLite extension manager using Boost.DLL
 * 
 * Manages dynamic loading of SQLite extensions with thread safety
 * and proper error handling.
 */
class SQLiteExtensionManager {
public:
    using ExtensionInitFunc = int (*)(sqlite3*, char**, const sqlite3_api_routines*);
    
    struct Extension {
        std::string name;
        fs::path path;
        // std::unique_ptr<boost::dll::shared_library> library; // Simplified for now
        ExtensionInitFunc init_func;
        bool loaded;
    };

    /**
     * @brief Constructor
     */
    SQLiteExtensionManager() = default;

    /**
     * @brief Add extension path for searching
     * @param path Directory containing SQLite extensions
     */
    void add_extension_path(const fs::path& path) {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (fs::exists(path) && fs::is_directory(path)) {
            extension_paths_.push_back(fs::canonical(path));
        }
    }

    /**
     * @brief Register an extension for loading
     * @param name Extension name (without file extension)
     * @param required If true, failure to load will throw exception
     * @return true if extension was found
     */
    bool register_extension(const std::string& name, bool required = false) {
        boost::lock_guard<boost::mutex> lock(mutex_);
        
        // Search for extension in registered paths
        fs::path ext_path = find_extension(name);
        if (ext_path.empty()) {
            if (required) {
                throw std::runtime_error("Required SQLite extension not found: " + name);
            }
            return false;
        }

        // Add to registered extensions
        auto ext = std::make_unique<Extension>();
        ext->name = name;
        ext->path = ext_path;
        ext->loaded = false;
        
        extensions_[name] = std::move(ext);
        return true;
    }

    /**
     * @brief Load all registered extensions into SQLite connection
     * @param db SQLite database connection
     * @throws std::runtime_error if loading fails
     */
    void load_extensions(sqlite3* db) {
        boost::lock_guard<boost::mutex> lock(mutex_);
        
        // Enable extension loading
        sqlite3_enable_load_extension(db, 1);
        
        // Create a scope guard to disable extension loading on exit
        struct CleanupGuard {
            sqlite3* db;
            ~CleanupGuard() {
                if (db) {
                    sqlite3_enable_load_extension(db, 0);
                }
            }
        };
        
        CleanupGuard cleanup_guard{db};
        
        try {
            for (auto& [name, ext] : extensions_) {
                if (!ext->loaded) {
                    load_extension_internal(db, *ext);
                }
            }
        } catch (...) {
            throw;
        }
    }

    /**
     * @brief Get list of loaded extensions
     * @return Vector of loaded extension names
     */
    std::vector<std::string> get_loaded_extensions() const {
        boost::lock_guard<boost::mutex> lock(mutex_);
        std::vector<std::string> loaded;
        
        for (const auto& [name, ext] : extensions_) {
            if (ext->loaded) {
                loaded.push_back(name);
            }
        }
        
        return loaded;
    }

    /**
     * @brief Check if a specific extension is loaded
     * @param name Extension name
     * @return true if extension is loaded
     */
    bool is_extension_loaded(const std::string& name) const {
        boost::lock_guard<boost::mutex> lock(mutex_);
        auto it = extensions_.find(name);
        return it != extensions_.end() && it->second->loaded;
    }

private:
    /**
     * @brief Find extension file in registered paths
     * @param name Extension name
     * @return Path to extension or empty path if not found
     */
    fs::path find_extension(const std::string& name) const {
        // Platform-specific extension patterns
#ifdef _WIN32
        std::vector<std::string> patterns = {name + ".dll", "lib" + name + ".dll"};
#elif defined(__APPLE__)
        std::vector<std::string> patterns = {name + ".dylib", "lib" + name + ".dylib", 
                                            name + ".so", "lib" + name + ".so"};
#else
        std::vector<std::string> patterns = {name + ".so", "lib" + name + ".so"};
#endif

        // Search in all registered paths
        for (const auto& search_path : extension_paths_) {
            for (const auto& pattern : patterns) {
                fs::path full_path = search_path / pattern;
                if (fs::exists(full_path)) {
                    return fs::canonical(full_path);
                }
            }
        }

        // Also search in system paths
        const char* system_paths[] = {
            "/usr/lib/sqlite3",
            "/usr/local/lib/sqlite3",
            "/opt/local/lib/sqlite3",
            nullptr
        };

        for (int i = 0; system_paths[i]; ++i) {
            fs::path search_path(system_paths[i]);
            if (fs::exists(search_path)) {
                for (const auto& pattern : patterns) {
                    fs::path full_path = search_path / pattern;
                    if (fs::exists(full_path)) {
                        return fs::canonical(full_path);
                    }
                }
            }
        }

        return fs::path();
    }

    /**
     * @brief Load a single extension
     * @param db SQLite database connection
     * @param ext Extension to load
     */
    void load_extension_internal(sqlite3* db, Extension& ext) {
        // Use SQLite's built-in extension loading mechanism
        char* error_msg = nullptr;
        int result = sqlite3_load_extension(db, ext.path.string().c_str(), nullptr, &error_msg);
        
        if (result == SQLITE_OK) {
            ext.loaded = true;
        } else {
            std::string error = error_msg ? error_msg : "Unknown error";
            sqlite3_free(error_msg);
            throw std::runtime_error("Failed to load extension " + ext.name + ": " + error);
        }
    }

    mutable boost::mutex mutex_;
    std::vector<fs::path> extension_paths_;
    std::map<std::string, std::unique_ptr<Extension>> extensions_;
};

} // namespace liarsdice::database

#endif // LIARSDICE_SQLITE_EXTENSIONS_HPP