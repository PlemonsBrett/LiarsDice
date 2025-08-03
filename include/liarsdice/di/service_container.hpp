#pragma once

#include <memory>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <functional>
#include <stdexcept>
#include <concepts>
#include <iostream>

namespace liarsdice::di {

// Service lifetime management
enum class ServiceLifetime {
    Transient,  // New instance every time
    Singleton,  // Single instance for application lifetime
    Scoped      // Single instance per scope (future use)
};

// Stream operator for ServiceLifetime (needed for Boost.Test)
inline std::ostream& operator<<(std::ostream& os, ServiceLifetime lifetime) {
    switch (lifetime) {
        case ServiceLifetime::Transient: return os << "Transient";
        case ServiceLifetime::Singleton: return os << "Singleton";
        case ServiceLifetime::Scoped: return os << "Scoped";
        default: return os << "Unknown";
    }
}

// Exception for DI container errors
class ServiceContainerException : public std::runtime_error {
public:
    explicit ServiceContainerException(const std::string& message) 
        : std::runtime_error("ServiceContainer: " + message) {}
};

// Factory function type
template<typename T>
using ServiceFactory = std::function<std::shared_ptr<T>()>;

// ServiceContainer implementation
class ServiceContainer {
public:
    ServiceContainer() = default;
    ~ServiceContainer() = default;
    
    // Non-copyable, non-movable (singleton pattern)
    ServiceContainer(const ServiceContainer&) = delete;
    ServiceContainer& operator=(const ServiceContainer&) = delete;
    ServiceContainer(ServiceContainer&&) = delete;
    ServiceContainer& operator=(ServiceContainer&&) = delete;

    // Register a service with factory function
    template<typename TInterface, typename TImplementation>
    void register_service(ServiceLifetime lifetime = ServiceLifetime::Transient) {
        static_assert(std::is_base_of_v<TInterface, TImplementation>, 
                     "TImplementation must inherit from TInterface");
        
        auto factory = []() -> std::shared_ptr<TInterface> {
            return std::make_shared<TImplementation>();
        };
        
        register_factory<TInterface>(std::move(factory), lifetime);
    }
    
    // Register with custom factory
    template<typename TInterface>
    void register_factory(ServiceFactory<TInterface> factory, 
                         ServiceLifetime lifetime = ServiceLifetime::Transient) {
        auto type_id = std::type_index(typeid(TInterface));
        
        ServiceInfo info;
        info.lifetime = lifetime;
        info.factory = [factory]() -> std::shared_ptr<void> {
            return std::static_pointer_cast<void>(factory());
        };
        
        services_[type_id] = std::move(info);
    }
    
    // Register singleton instance
    template<typename TInterface>
    void register_instance(std::shared_ptr<TInterface> instance) {
        auto type_id = std::type_index(typeid(TInterface));
        
        ServiceInfo info;
        info.lifetime = ServiceLifetime::Singleton;
        info.instance = std::static_pointer_cast<void>(instance);
        
        services_[type_id] = std::move(info);
    }
    
    // Resolve service
    template<typename T>
    std::shared_ptr<T> resolve() {
        auto type_id = std::type_index(typeid(T));
        
        auto it = services_.find(type_id);
        if (it == services_.end()) {
            throw ServiceContainerException("Service not registered: " + 
                                           std::string(typeid(T).name()));
        }
        
        auto& info = it->second;
        
        switch (info.lifetime) {
            case ServiceLifetime::Singleton:
                if (!info.instance && info.factory) {
                    info.instance = info.factory();
                }
                return std::static_pointer_cast<T>(info.instance);
                
            case ServiceLifetime::Transient:
                if (!info.factory) {
                    throw ServiceContainerException("No factory for transient service: " + 
                                                   std::string(typeid(T).name()));
                }
                return std::static_pointer_cast<T>(info.factory());
                
            case ServiceLifetime::Scoped:
                // For now, treat as singleton (future enhancement)
                if (!info.instance && info.factory) {
                    info.instance = info.factory();
                }
                return std::static_pointer_cast<T>(info.instance);
        }
        
        throw ServiceContainerException("Failed to resolve service: " + 
                                       std::string(typeid(T).name()));
    }
    
    // Check if service is registered
    template<typename T>
    bool is_registered() const {
        auto type_id = std::type_index(typeid(T));
        return services_.find(type_id) != services_.end();
    }
    
    // Clear all services
    void clear() {
        services_.clear();
    }
    
    // Get service count
    size_t service_count() const {
        return services_.size();
    }

private:
    struct ServiceInfo {
        ServiceLifetime lifetime;
        std::function<std::shared_ptr<void>()> factory;
        mutable std::shared_ptr<void> instance; // mutable for lazy singleton creation
    };
    
    std::unordered_map<std::type_index, ServiceInfo> services_;
};

// Global container instance (singleton)
ServiceContainer& get_service_container();

// Convenience macros for dependency injection
#define REGISTER_SERVICE(container, interface, implementation) \
    container.register_service<interface, implementation>()

#define REGISTER_SINGLETON(container, interface, implementation) \
    container.register_service<interface, implementation>(ServiceLifetime::Singleton)

#define RESOLVE_SERVICE(container, type) \
    container.resolve<type>()

} // namespace liarsdice::di