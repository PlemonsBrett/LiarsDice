#pragma once

// Main dependency injection header - includes all DI components
#include "service_container.hpp"

namespace liarsdice::di {

/**
 * @brief Global service locator for application-wide services
 *
 * Provides a singleton pattern for accessing the main service container.
 * Use sparingly - prefer constructor injection where possible.
 */
class ServiceLocator {
private:
  static inline std::unique_ptr<ServiceContainer> container;
  static inline bool initialized = false;

public:
  /**
   * @brief Initialize the service locator with a container
   */
  static void initialize(std::unique_ptr<ServiceContainer> new_container) {
    ServiceLocator::container = std::move(new_container);
    initialized = true;
  }

  /**
   * @brief Get the global service container
   */
  static ServiceContainer &get_container() {
    if (!initialized || !container) {
      throw DIException("ServiceLocator not initialized", DIError::kServiceNotRegistered);
    }
    return *container;
  }

  /**
   * @brief Resolve a service from the global container
   */
  template <interfaces::ServiceInterface T> static auto resolve() {
    return get_container().resolve<T>();
  }

  /**
   * @brief Resolve a named service from the global container
   */
  template <interfaces::ServiceInterface T> static auto resolve(const std::string &name) {
    return get_container().resolve<T>(name);
  }

  /**
   * @brief Check if the service locator is initialized
   */
  static bool is_initialized() { return initialized && container != nullptr; }

  /**
   * @brief Reset the service locator
   */
  static void reset() {
    container.reset();
    initialized = false;
  }
};

/**
 * @brief Helper functions for common DI operations
 */
template <typename Container, typename Interface, typename Implementation, typename... Args>
constexpr void register_service(Container &container, Args &&...args) {
  container.template register_service<Interface, Implementation>(std::forward<Args>(args)...);
}

template <typename Container, typename Interface, typename Implementation, typename... Args>
constexpr void register_singleton(Container &container, Args &&...args) {
  container.template register_service<Interface, Implementation>(
      ::liarsdice::di::ServiceLifetime::kSingleton, "", std::forward<Args>(args)...);
}

template <typename Container, typename Interface>
constexpr auto resolve_service(Container &container) {
  return container.template resolve<Interface>();
}

template <typename Container, typename Interface>
constexpr auto resolve_named_service(Container &container, const std::string &name) {
  return container.template resolve<Interface>(name);
}

} // namespace liarsdice::di