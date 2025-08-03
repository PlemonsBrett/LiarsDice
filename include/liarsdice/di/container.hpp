#pragma once

#include <boost/di.hpp>
#include <memory>

namespace liarsdice::di {

// Boost.DI namespace alias
namespace di = boost::di;

// Common DI policies and configurations
using injector_config = di::config;

// Convenience type aliases
template<typename T>
using shared = std::shared_ptr<T>;

template<typename T>
using unique = std::unique_ptr<T>;

// Scopes
constexpr auto singleton = di::singleton;
constexpr auto unique_scope = di::unique;

// Factory function helper
template<typename T, typename... Args>
auto make_factory() {
    return [](Args... args) { return std::make_shared<T>(args...); };
}

// Binding helpers
template<typename Interface, typename Implementation>
auto bind_interface() {
    return di::bind<Interface>().to<Implementation>();
}

template<typename Interface, typename Implementation>
auto bind_singleton() {
    return di::bind<Interface>().to<Implementation>().in(singleton);
}

template<typename T>
auto bind_instance(std::shared_ptr<T> instance) {
    return di::bind<T>().to(instance);
}

// Configuration helper
template<typename... Bindings>
auto make_injector(Bindings&&... bindings) {
    return di::make_injector(std::forward<Bindings>(bindings)...);
}

// Testing helpers
namespace testing {
    
    // Mock binding helper for tests
    template<typename Interface, typename Mock>
    auto bind_mock() {
        return di::bind<Interface>().to<Mock>().in(singleton);
    }
    
    // Test injector with mocks
    template<typename... MockBindings>
    auto make_test_injector(MockBindings&&... bindings) {
        return di::make_injector(std::forward<MockBindings>(bindings)...);
    }
}

} // namespace liarsdice::di

// Convenience macros for cleaner syntax
#define INJECT(...) BOOST_DI_INJECT(__VA_ARGS__)

#define DI_BIND(interface, implementation) \
    liarsdice::di::bind_interface<interface, implementation>()

#define DI_BIND_SINGLETON(interface, implementation) \
    liarsdice::di::bind_singleton<interface, implementation>()

#define DI_BIND_INSTANCE(type, instance) \
    liarsdice::di::bind_instance<type>(instance)

#define DI_MAKE_INJECTOR(...) \
    liarsdice::di::make_injector(__VA_ARGS__)

#define DI_CREATE(injector, type) \
    injector.create<std::shared_ptr<type>>()

#define DI_TEST_BIND(interface, mock) \
    liarsdice::di::testing::bind_mock<interface, mock>()