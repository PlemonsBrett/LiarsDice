#pragma once

// Main dependency injection header - includes all DI components
#include "service_container.hpp"
#include "service_factory.hpp"

namespace liarsdice::di {

/**
 * @brief Global service locator for application-wide services
 * 
 * Provides a singleton pattern for accessing the main service container.
 * Use sparingly - prefer constructor injection where possible.
 */
class ServiceLocator {
private:
    static inline std::unique_ptr<ServiceContainer> container_;
    static inline bool initialized_ = false;

public:
    /**
     * @brief Initialize the service locator with a container
     */
    static void initialize(std::unique_ptr<ServiceContainer> container) {
        container_ = std::move(container);
        initialized_ = true;
    }

    /**
     * @brief Get the global service container
     */
    static ServiceContainer& get_container() {
        if (!initialized_ || !container_) {
            throw DIException("ServiceLocator not initialized", DIError::ServiceNotRegistered);
        }
        return *container_;
    }

    /**
     * @brief Resolve a service from the global container
     */
    template<interfaces::ServiceInterface T>
    static auto resolve() {
        return get_container().resolve<T>();
    }

    /**
     * @brief Resolve a named service from the global container
     */
    template<interfaces::ServiceInterface T>
    static auto resolve(const std::string& name) {
        return get_container().resolve<T>(name);
    }

    /**
     * @brief Check if the service locator is initialized
     */
    static bool is_initialized() {
        return initialized_ && container_ != nullptr;
    }

    /**
     * @brief Reset the service locator
     */
    static void reset() {
        container_.reset();
        initialized_ = false;
    }
};

/**
 * @brief Helper macros for common DI operations
 */
#define REGISTER_SERVICE(Container, Interface, Implementation, ...) \
    (Container).register_service<Interface, Implementation>(__VA_ARGS__)

#define REGISTER_SINGLETON(Container, Interface, Implementation, ...) \
    (Container).register_service<Interface, Implementation>( \
        ::liarsdice::di::ServiceLifetime::Singleton, "", __VA_ARGS__)

#define RESOLVE_SERVICE(Container, Interface) \
    (Container).resolve<Interface>()

#define RESOLVE_NAMED_SERVICE(Container, Interface, Name) \
    (Container).resolve<Interface>(Name)

} // namespace liarsdice::di