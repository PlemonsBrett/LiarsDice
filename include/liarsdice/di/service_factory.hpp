#pragma once

#include <memory>
#include <functional>
#include "../interfaces/concepts.hpp"
#include "service_container.hpp"

namespace liarsdice::di {

/**
 * @brief Factory base class for creating services
 * 
 * @tparam T Service type to create
 */
template<interfaces::ServiceInterface T>
class IServiceFactory {
public:
    virtual ~IServiceFactory() = default;
    virtual std::unique_ptr<T> create() = 0;
    virtual std::unique_ptr<T> create(ServiceContainer& container) = 0;
};

/**
 * @brief Concrete factory implementation for services
 * 
 * @tparam TInterface Interface type
 * @tparam TImplementation Implementation type
 */
template<interfaces::ServiceInterface TInterface, 
         std::derived_from<TInterface> TImplementation>
class ServiceFactory : public IServiceFactory<TInterface> {
private:
    std::function<std::unique_ptr<TImplementation>()> factory_func_;

public:
    /**
     * @brief Construct factory with default constructor
     */
    ServiceFactory() 
        : factory_func_([]() { return std::make_unique<TImplementation>(); }) {}

    /**
     * @brief Construct factory with custom creation function
     * 
     * @param factory Custom factory function
     */
    explicit ServiceFactory(std::function<std::unique_ptr<TImplementation>()> factory)
        : factory_func_(std::move(factory)) {}

    /**
     * @brief Create instance without container dependencies
     * 
     * @return Unique pointer to created service
     */
    std::unique_ptr<TInterface> create() override {
        return factory_func_();
    }

    /**
     * @brief Create instance with container for dependency injection
     * 
     * @param container Service container for resolving dependencies
     * @return Unique pointer to created service
     */
    std::unique_ptr<TInterface> create(ServiceContainer& /* container */) override {
        // In a more advanced implementation, this could resolve constructor dependencies
        // For now, we'll just call the factory function
        return factory_func_();
    }
};

/**
 * @brief Factory for services with dependencies that need to be resolved
 * 
 * @tparam TInterface Interface type
 * @tparam TImplementation Implementation type
 * @tparam TDeps Dependency types
 */
template<interfaces::ServiceInterface TInterface, 
         std::derived_from<TInterface> TImplementation,
         interfaces::ServiceInterface... TDeps>
class DependentServiceFactory : public IServiceFactory<TInterface> {
public:
    /**
     * @brief Create instance without external container (will fail if dependencies needed)
     */
    std::unique_ptr<TInterface> create() override {
        static_assert(sizeof...(TDeps) == 0, 
            "Cannot create service with dependencies without container");
        return std::make_unique<TImplementation>();
    }

    /**
     * @brief Create instance with dependency resolution
     * 
     * @param container Service container for resolving dependencies
     * @return Unique pointer to created service
     */
    std::unique_ptr<TInterface> create(ServiceContainer& container) override {
        return create_with_dependencies(container);
    }

private:
    std::unique_ptr<TInterface> create_with_dependencies(ServiceContainer& container) {
        if constexpr (sizeof...(TDeps) == 0) {
            return std::make_unique<TImplementation>();
        } else {
            // Resolve all dependencies
            auto resolved_deps = resolve_dependencies(container);
            return create_with_resolved_deps(std::move(resolved_deps));
        }
    }

    auto resolve_dependencies(ServiceContainer& container) {
        return std::make_tuple(container.resolve<TDeps>()...);
    }

    template<typename... ResolvedDeps>
    std::unique_ptr<TInterface> create_with_resolved_deps(
        std::tuple<ServiceContainer::ServiceResult<TDeps>...> deps) {
        
        // Check if all dependencies were resolved successfully
        auto check_deps = [](const auto&... dep_results) {
            return (dep_results.has_value() && ...);
        };
        
        if (!std::apply(check_deps, deps)) {
            throw DIException("Failed to resolve dependencies", DIError::kCreationFailed);
        }

        // Extract values and create instance
        auto extract_and_create = [](auto&... dep_results) {
            return std::make_unique<TImplementation>(std::move(*dep_results)...);
        };

        return std::apply(extract_and_create, deps);
    }
};

/**
 * @brief Builder class for easy service registration
 */
template<interfaces::ServiceInterface TInterface>
class ServiceRegistration {
private:
    ServiceContainer* container_;
    ServiceLifetime lifetime_ = ServiceLifetime::kTransient;
    std::string name_;

public:
    explicit ServiceRegistration(ServiceContainer& container) 
        : container_(&container) {}

    /**
     * @brief Set service lifetime
     */
    ServiceRegistration& as_singleton() {
        lifetime_ = ServiceLifetime::kSingleton;
        return *this;
    }

    ServiceRegistration& as_transient() {
        lifetime_ = ServiceLifetime::kTransient;
        return *this;
    }

    /**
     * @brief Set service name
     */
    ServiceRegistration& named(const std::string& name) {
        name_ = name;
        return *this;
    }

    /**
     * @brief Register implementation
     */
    template<std::derived_from<TInterface> TImplementation, typename... Args>
    void use(Args&&... args) {
        container_->register_service<TInterface, TImplementation>(
            lifetime_, name_, std::forward<Args>(args)...);
    }

    /**
     * @brief Register with factory function
     */
    void use_factory(std::function<std::unique_ptr<TInterface>()> factory) {
        container_->register_factory<TInterface>(std::move(factory), lifetime_, name_);
    }

    /**
     * @brief Register existing instance
     */
    void use_instance(std::unique_ptr<TInterface> instance) {
        container_->register_instance<TInterface>(std::move(instance), name_);
    }
};

/**
 * @brief Fluent interface for service container configuration
 */
class ServiceContainerBuilder {
private:
    std::unique_ptr<ServiceContainer> container_;

public:
    ServiceContainerBuilder() : container_(std::make_unique<ServiceContainer>()) {}

    /**
     * @brief Begin registration for a service interface
     */
    template<interfaces::ServiceInterface TInterface>
    ServiceRegistration<TInterface> register_service() {
        return ServiceRegistration<TInterface>(*container_);
    }

    /**
     * @brief Build and return the configured container
     */
    std::unique_ptr<ServiceContainer> build() {
        return std::move(container_);
    }

    /**
     * @brief Get reference to container for direct access
     */
    ServiceContainer& get_container() {
        return *container_;
    }
};

} // namespace liarsdice::di