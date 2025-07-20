#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <string>
#include <vector>

namespace liarsdice::di {

/**
 * @brief Simplified Dependency Injection Container
 * 
 * A simplified, working implementation that focuses on core functionality
 * without the complexity of std::any and advanced type erasure.
 */
class SimpleContainer {
public:
    // Factory function that creates void* (to be cast to the correct type)
    using FactoryFunction = std::function<void*()>;
    using DeleterFunction = std::function<void(void*)>;

private:
    struct ServiceEntry {
        FactoryFunction factory;
        DeleterFunction deleter;
        std::string name;
        
        ServiceEntry(FactoryFunction f, DeleterFunction d, std::string n)
            : factory(std::move(f)), deleter(std::move(d)), name(std::move(n)) {}
    };

    std::unordered_map<std::type_index, ServiceEntry> services_;
    std::unordered_map<std::string, std::type_index> named_services_;

public:
    SimpleContainer() = default;
    ~SimpleContainer() = default;

    // Non-copyable but movable
    SimpleContainer(const SimpleContainer&) = delete;
    SimpleContainer& operator=(const SimpleContainer&) = delete;
    SimpleContainer(SimpleContainer&&) = default;
    SimpleContainer& operator=(SimpleContainer&&) = default;

    /**
     * @brief Register a service type
     */
    template<typename TInterface, typename TImplementation, typename... Args>
    void register_service(const std::string& name = "", Args&&... args) {
        static_assert(std::is_base_of_v<TInterface, TImplementation>, 
                     "TImplementation must derive from TInterface");

        auto factory = [args = std::tuple<Args...>(std::forward<Args>(args)...)]() -> void* {
            return std::apply([](auto&&... args) { return new TImplementation(std::forward<decltype(args)>(args)...); }, args);
        };

        auto deleter = [](void* ptr) {
            std::unique_ptr<TImplementation>(static_cast<TImplementation*>(ptr));
        };

        auto type_index = std::type_index(typeid(TInterface));
        auto service_name = name.empty() ? typeid(TInterface).name() : name;

        services_.emplace(type_index, ServiceEntry(factory, deleter, service_name));

        if (!name.empty()) {
            named_services_.emplace(name, type_index);
        }
    }

    /**
     * @brief Register a factory function
     */
    template<typename TInterface>
    void register_factory(std::function<std::unique_ptr<TInterface>()> factory,
                         const std::string& name = "") {
        
        auto wrapper_factory = [factory = std::move(factory)]() -> void* {
            auto ptr = factory();
            return ptr.release();  // Release ownership to raw pointer
        };

        auto deleter = [](void* ptr) {
            std::unique_ptr<TInterface>(static_cast<TInterface*>(ptr));
        };

        auto type_index = std::type_index(typeid(TInterface));
        auto service_name = name.empty() ? typeid(TInterface).name() : name;

        services_.emplace(type_index, ServiceEntry(wrapper_factory, deleter, service_name));

        if (!name.empty()) {
            named_services_.emplace(name, type_index);
        }
    }

    /**
     * @brief Resolve a service by type
     */
    template<typename T>
    std::unique_ptr<T> resolve() {
        auto type_index = std::type_index(typeid(T));
        auto it = services_.find(type_index);
        
        if (it == services_.end()) {
            return nullptr;
        }

        void* raw_ptr = it->second.factory();
        return std::unique_ptr<T>(static_cast<T*>(raw_ptr));
    }

    /**
     * @brief Resolve a service by name
     */
    template<typename T>
    std::unique_ptr<T> resolve(const std::string& name) {
        auto it = named_services_.find(name);
        if (it == named_services_.end()) {
            return nullptr;
        }

        auto service_it = services_.find(it->second);
        if (service_it == services_.end()) {
            return nullptr;
        }

        void* raw_ptr = service_it->second.factory();
        return std::unique_ptr<T>(static_cast<T*>(raw_ptr));
    }

    /**
     * @brief Check if a service is registered
     */
    template<typename T>
    bool is_registered() const {
        auto type_index = std::type_index(typeid(T));
        return services_.contains(type_index);
    }

    /**
     * @brief Check if a named service is registered
     */
    bool is_registered(const std::string& name) const {
        return named_services_.contains(name);
    }

    /**
     * @brief Get all registered service names
     */
    std::vector<std::string> get_registered_services() const {
        std::vector<std::string> names;
        names.reserve(named_services_.size());
        
        for (const auto& [name, _] : named_services_) {
            names.push_back(name);
        }
        
        return names;
    }

    /**
     * @brief Get the number of registered services
     */
    size_t size() const {
        return services_.size();
    }

    /**
     * @brief Clear all services
     */
    void clear() {
        services_.clear();
        named_services_.clear();
    }
};

} // namespace liarsdice::di