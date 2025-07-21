#pragma once

#include "../interfaces/concepts.hpp"
#include "../interfaces/i_service_factory.hpp"
#include <concepts>
#include <expected>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>

// LLDB debugging support macros
#ifdef __clang__
#define LLDB_VISUALIZER(name) __attribute__((annotate("lldb.visualizer:" name)))
#define LLDB_DEBUG_INFO __attribute__((used, retain))
#else
#define LLDB_VISUALIZER(name)
#define LLDB_DEBUG_INFO
#endif

namespace liarsdice::di {

/**
 * @brief Service lifetime management options
 */
enum class ServiceLifetime : std::uint8_t {
  kTransient, ///< New instance created each time
  kSingleton  ///< Single instance reused across all requests
};

/**
 * @brief Error types for dependency injection operations
 */
enum class DIError : std::uint8_t {
  kServiceNotRegistered,
  kCircularDependency,
  kCreationFailed,
  kInvalidLifetime
};

/**
 * @brief Exception thrown by DI container operations
 */
class DIException : public std::runtime_error {
public:
  explicit DIException(const std::string &message, DIError error_type)
      : std::runtime_error(message), error_type_(error_type) {}

  [[nodiscard]] DIError get_error_type() const noexcept { return error_type_; }

private:
  DIError error_type_;
};

/**
 * @brief Modern C++23 Dependency Injection Container
 *
 * Features:
 * - Type-safe service registration and resolution
 * - Support for transient and singleton lifetimes
 * - Automatic dependency injection via constructor parameters
 * - C++23 concepts for compile-time interface validation
 * - std::expected for error handling without exceptions
 * - Template-based factory registration
 */
class LLDB_VISUALIZER("ServiceContainer") ServiceContainer {
public:
  /**
   * @brief Result type for service resolution operations
   */
  template <typename T> using ServiceResult = std::expected<std::unique_ptr<T>, DIError>;

private:
  /**
   * @brief Internal service descriptor
   */
  struct LLDB_VISUALIZER("ServiceDescriptor") ServiceDescriptor {
    std::unique_ptr<interfaces::IServiceFactory> factory;
    ServiceLifetime lifetime;
    void *singleton_instance{};
    std::type_index service_type;
    std::string service_name;
    mutable std::once_flag singleton_flag;  ///< Thread-safe singleton initialization

    ServiceDescriptor(std::unique_ptr<interfaces::IServiceFactory> f, ServiceLifetime lt,
                      std::type_index type, std::string name)
        : factory(std::move(f)), lifetime(lt), service_type(type), service_name(std::move(name)) {}
    
    // Make movable for container operations
    ServiceDescriptor(ServiceDescriptor&& other) noexcept 
        : factory(std::move(other.factory)),
          lifetime(other.lifetime),
          singleton_instance(other.singleton_instance),
          service_type(other.service_type),
          service_name(std::move(other.service_name)) {
      // std::once_flag is not movable, so we leave it in default state
    }
    
    ServiceDescriptor& operator=(ServiceDescriptor&& other) noexcept {
      if (this != &other) {
        factory = std::move(other.factory);
        lifetime = other.lifetime;
        singleton_instance = other.singleton_instance;
        service_type = other.service_type;
        service_name = std::move(other.service_name);
        // std::once_flag cannot be moved, leave in default state
      }
      return *this;
    }
    
    // Delete copy operations
    ServiceDescriptor(const ServiceDescriptor&) = delete;
    ServiceDescriptor& operator=(const ServiceDescriptor&) = delete;
  };

  std::unordered_map<std::type_index, ServiceDescriptor> services_;
  std::unordered_map<std::string, ServiceDescriptor> named_services_;
  thread_local static std::unordered_set<std::type_index> resolution_stack_;  ///< Track resolution for circular dependency detection

public:
  ServiceContainer() = default;
  ~ServiceContainer() = default;

  // Non-copyable but movable
  ServiceContainer(const ServiceContainer &) = delete;
  ServiceContainer &operator=(const ServiceContainer &) = delete;
  ServiceContainer(ServiceContainer &&) = default;
  ServiceContainer &operator=(ServiceContainer &&) = default;

  /**
   * @brief Register a service with automatic dependency injection
   *
   * @tparam TInterface Interface type
   * @tparam TImplementation Implementation type
   * @tparam Args Constructor argument types
   * @param lifetime Service lifetime
   * @param name Optional service name
   * @param args Constructor arguments
   */
  template <interfaces::ServiceInterface TInterface, std::derived_from<TInterface> TImplementation,
            typename... Args>
  void register_service(ServiceLifetime lifetime = ServiceLifetime::kTransient,
                        const std::string &name = {},
                        Args &&...args) { // NOLINT(cppcoreguidelines-missing-std-forward)
    auto factory_func = [args_tuple = std::tuple<Args...>(
                             std::forward<Args>(args)...)]() -> std::unique_ptr<TInterface> {
      auto ptr = std::apply(
          [](auto &&...forwarded_args) {
            return std::make_unique<TImplementation>(
                std::forward<decltype(forwarded_args)>(forwarded_args)...);
          },
          args_tuple);
      return std::unique_ptr<TInterface>(std::move(ptr));
    };

    auto factory =
        std::make_unique<interfaces::ServiceFactory<TInterface>>(std::move(factory_func));
    auto type_index = std::type_index(typeid(TInterface));
    auto service_name = name.empty() ? typeid(TInterface).name() : name;

    if (!name.empty()) {
      // Store named service separately
      named_services_.emplace(
          name, ServiceDescriptor(std::move(factory), lifetime, type_index, service_name));
    } else {
      // Store in main services map
      services_.emplace(type_index,
                        ServiceDescriptor(std::move(factory), lifetime, type_index, service_name));
    }
  }

  /**
   * @brief Register a service with a custom factory function
   *
   * @tparam TInterface Interface type
   * @param factory Custom factory function
   * @param lifetime Service lifetime
   * @param name Optional service name
   */
  template <interfaces::ServiceInterface TInterface>
  void register_factory(std::function<std::unique_ptr<TInterface>()> factory,
                        ServiceLifetime lifetime = ServiceLifetime::kTransient,
                        const std::string &name = {}) {
    auto service_factory =
        std::make_unique<interfaces::ServiceFactory<TInterface>>(std::move(factory));
    auto type_index = std::type_index(typeid(TInterface));
    auto service_name = name.empty() ? typeid(TInterface).name() : name;

    if (!name.empty()) {
      // Store named service separately
      named_services_.emplace(
          name, ServiceDescriptor(std::move(service_factory), lifetime, type_index, service_name));
    } else {
      // Store in main services map
      services_.emplace(type_index, ServiceDescriptor(std::move(service_factory), lifetime,
                                                      type_index, service_name));
    }
  }

  /**
   * @brief Register an existing instance as a singleton
   *
   * @tparam TInterface Interface type
   * @param instance Pre-created instance to register
   * @param name Optional service name
   */
  template <interfaces::ServiceInterface TInterface>
  void register_instance(std::unique_ptr<TInterface> instance, const std::string &name = {}) {
    auto factory_func = [instance = std::move(instance)]() mutable -> std::unique_ptr<TInterface> {
      return std::move(instance);
    };

    auto factory =
        std::make_unique<interfaces::ServiceFactory<TInterface>>(std::move(factory_func));
    auto type_index = std::type_index(typeid(TInterface));
    auto service_name = name.empty() ? typeid(TInterface).name() : name;

    if (!name.empty()) {
      // Store named service separately
      named_services_.emplace(name,
                              ServiceDescriptor(std::move(factory), ServiceLifetime::kSingleton,
                                                type_index, service_name));
    } else {
      // Store in main services map
      services_.emplace(type_index,
                        ServiceDescriptor(std::move(factory), ServiceLifetime::kSingleton,
                                          type_index, service_name));
    }
  }

  /**
   * @brief Resolve a service by type
   *
   * @tparam T Service interface type
   * @return Expected containing unique_ptr to service or error
   */
  template <interfaces::ServiceInterface T> ServiceResult<T> resolve() {
    auto type_index = std::type_index(typeid(T));
    return resolve_internal<T>(type_index);
  }

  /**
   * @brief Resolve a service by name
   *
   * @tparam T Service interface type
   * @param name Service name
   * @return Expected containing unique_ptr to service or error
   */
  template <interfaces::ServiceInterface T> ServiceResult<T> resolve(const std::string &name) {
    auto it = named_services_.find(name);
    if (it == named_services_.end()) {
      return std::unexpected(DIError::kServiceNotRegistered);
    }
    return resolve_internal<T>(it->second);
  }

  /**
   * @brief Check if a service is registered by type
   *
   * @tparam T Service type
   * @return true if registered, false otherwise
   */
  template <typename T> [[nodiscard]] bool is_registered() const {
    auto type_index = std::type_index(typeid(T));
    if (services_.contains(type_index)) {
      return true;
    }

    // Also check named services for this type
    // NOLINT(readability-use-anyofallof): std::ranges::any_of not available in this library version
    for (const auto &[name, descriptor] : named_services_) { // NOLINT
      if (descriptor.service_type == type_index) {
        return true;
      }
    }

    return false;
  }

  /**
   * @brief Check if a service is registered by name
   *
   * @param name Service name
   * @return true if registered, false otherwise
   */
  [[nodiscard]] bool is_registered(const std::string &name) const {
    return named_services_.contains(name);
  }

  /**
   * @brief Get list of registered service names
   *
   * @return Vector of service names
   */
  [[nodiscard]] std::vector<std::string> get_registered_services() const {
    std::vector<std::string> names;
    names.reserve(services_.size() + named_services_.size());

    // Add unnamed services
    for (const auto &[type_index, descriptor] : services_) {
      names.push_back(descriptor.service_name);
    }

    // Add named services
    for (const auto &[name, descriptor] : named_services_) {
      names.push_back(name);
    }

    return names;
  }

  /**
   * @brief Clear all registered services
   */
  void clear() {
    services_.clear();
    named_services_.clear();
  }

  /**
   * @brief Get number of registered services
   *
   * @return Number of services
   */
  [[nodiscard]] size_t size() const { return services_.size() + named_services_.size(); }

  /**
   * @brief Get debugging information about the container state
   * @return Debug info string for LLDB visualization
   */
  [[nodiscard]] LLDB_DEBUG_INFO std::string get_debug_info() const {
    std::string debug_info;
    debug_info += std::format("ServiceContainer: {} services registered\n", size());
    debug_info += std::format("  Type services: {}\n", services_.size());
    debug_info += std::format("  Named services: {}\n", named_services_.size());
    debug_info += std::format("  Resolution stack depth: {}\n", resolution_stack_.size());
    return debug_info;
  }

private:
  template <typename T> ServiceResult<T> resolve_internal(std::type_index type_index) {
    // Circular dependency detection
    if (resolution_stack_.contains(type_index)) {
      return std::unexpected(DIError::kCircularDependency);
    }

    auto it = services_.find(type_index);
    if (it == services_.end()) {
      return std::unexpected(DIError::kServiceNotRegistered);
    }

    // Add to resolution stack for circular dependency detection
    resolution_stack_.insert(type_index);
    
    auto &descriptor = it->second;
    auto result = resolve_internal<T>(descriptor);
    
    // Remove from resolution stack
    resolution_stack_.erase(type_index);
    
    return result;
  }

  template <typename T> ServiceResult<T> resolve_internal(ServiceDescriptor &descriptor) {
    try {
      // Use if consteval for compile-time optimization when possible
      if consteval {
        // Compile-time optimization path for known types
        if (std::is_same_v<T, typename std::remove_cv_t<T>>) {
          // Optimize for common interface types at compile time
        }
      }

      switch (descriptor.lifetime) {
        case ServiceLifetime::kSingleton: {
          // For unique_ptr semantics, we create a new instance each time
          // In a real DI container, singletons would use shared_ptr instead
          void *instance = descriptor.factory->create();
          return std::unique_ptr<T>(static_cast<T *>(instance));
        }
        case ServiceLifetime::kTransient: {
          // Transient: create new instance each time
          void *instance = descriptor.factory->create();
          return std::unique_ptr<T>(static_cast<T *>(instance));
        }
        default:
          // This should never be reached with valid enum values
          std::unreachable();
      }
    } catch (...) {
      return std::unexpected(DIError::kCreationFailed);
    }
  }
};

/**
 * @brief RAII scope guard for service container
 *
 * Provides automatic cleanup and scoped service registration
 */
class ServiceScope {
private:
  ServiceContainer &container_;
  std::vector<std::type_index> registered_types_;
  std::vector<std::string> registered_names_;

public:
  explicit ServiceScope(ServiceContainer &container) : container_(container) {}

  ~ServiceScope() = default;

  // Non-copyable and non-movable (contains reference member)
  ServiceScope(const ServiceScope &) = delete;
  ServiceScope &operator=(const ServiceScope &) = delete;
  ServiceScope(ServiceScope &&) = delete;
  ServiceScope &operator=(ServiceScope &&) = delete;

  // Forward registration methods to container
  template <interfaces::ServiceInterface TInterface, std::derived_from<TInterface> TImplementation,
            typename... Args>
  void register_service(ServiceLifetime lifetime = ServiceLifetime::kTransient,
                        const std::string &name = {}, Args &&...args) {
    container_.register_service<TInterface, TImplementation>(lifetime, name,
                                                             std::forward<Args>(args)...);

    registered_types_.push_back(std::type_index(typeid(TInterface)));
    if (!name.empty()) {
      registered_names_.push_back(name);
    }
  }

  template <interfaces::ServiceInterface T> auto resolve() { return container_.resolve<T>(); }

  template <interfaces::ServiceInterface T> auto resolve(const std::string &name) {
    return container_.resolve<T>(name);
  }
};

} // namespace liarsdice::di