#define BOOST_TEST_MODULE BoostDITests
#include <boost/test/unit_test.hpp>
#include <liarsdice/di/container.hpp>
#include <memory>
#include <string>

using namespace liarsdice::di;

// Test interfaces and implementations
class ITestService {
public:
    virtual ~ITestService() = default;
    virtual std::string get_name() const = 0;
    virtual int get_value() const = 0;
};

class TestServiceImpl : public ITestService {
public:
    std::string get_name() const override { return "TestService"; }
    int get_value() const override { return 42; }
};

class IDependentService {
public:
    virtual ~IDependentService() = default;
    virtual std::string get_service_name() const = 0;
    virtual int get_total() const = 0;
};

class DependentServiceImpl : public IDependentService {
public:
    INJECT(DependentServiceImpl, std::shared_ptr<ITestService> service)
        : service_(std::move(service)) {}
    
    std::string get_service_name() const override {
        return "Dependent[" + service_->get_name() + "]";
    }
    
    int get_total() const override {
        return service_->get_value() + 100;
    }

private:
    std::shared_ptr<ITestService> service_;
};

BOOST_AUTO_TEST_SUITE(BoostDIBasicTests)

BOOST_AUTO_TEST_CASE(simple_binding_and_resolution) {
    auto injector = DI_MAKE_INJECTOR(
        DI_BIND(ITestService, TestServiceImpl)
    );
    
    auto service = DI_CREATE(injector, ITestService);
    
    BOOST_CHECK(service != nullptr);
    BOOST_CHECK_EQUAL(service->get_name(), "TestService");
    BOOST_CHECK_EQUAL(service->get_value(), 42);
}

BOOST_AUTO_TEST_CASE(singleton_binding) {
    auto injector = DI_MAKE_INJECTOR(
        DI_BIND_SINGLETON(ITestService, TestServiceImpl)
    );
    
    auto service1 = DI_CREATE(injector, ITestService);
    auto service2 = DI_CREATE(injector, ITestService);
    
    // Should be the same instance for singleton
    BOOST_CHECK_EQUAL(service1.get(), service2.get());
}

BOOST_AUTO_TEST_CASE(dependency_injection) {
    auto injector = DI_MAKE_INJECTOR(
        DI_BIND(ITestService, TestServiceImpl),
        DI_BIND(IDependentService, DependentServiceImpl)
    );
    
    auto dependent = DI_CREATE(injector, IDependentService);
    
    BOOST_CHECK(dependent != nullptr);
    BOOST_CHECK_EQUAL(dependent->get_service_name(), "Dependent[TestService]");
    BOOST_CHECK_EQUAL(dependent->get_total(), 142); // 42 + 100
}

BOOST_AUTO_TEST_CASE(instance_binding) {
    auto custom_service = std::make_shared<TestServiceImpl>();
    
    auto injector = DI_MAKE_INJECTOR(
        DI_BIND_INSTANCE(ITestService, custom_service)
    );
    
    auto resolved = DI_CREATE(injector, ITestService);
    
    BOOST_CHECK_EQUAL(custom_service.get(), resolved.get());
}

BOOST_AUTO_TEST_SUITE_END()

// Test suite for testing support
BOOST_AUTO_TEST_SUITE(BoostDITestingTests)

class MockTestService : public ITestService {
public:
    std::string get_name() const override { return "MockService"; }
    int get_value() const override { return 999; }
};

BOOST_AUTO_TEST_CASE(mock_binding_for_tests) {
    auto test_injector = testing::make_test_injector(
        DI_TEST_BIND(ITestService, MockTestService),
        DI_BIND(IDependentService, DependentServiceImpl)
    );
    
    auto dependent = DI_CREATE(test_injector, IDependentService);
    
    BOOST_CHECK_EQUAL(dependent->get_service_name(), "Dependent[MockService]");
    BOOST_CHECK_EQUAL(dependent->get_total(), 1099); // 999 + 100
}

BOOST_AUTO_TEST_SUITE_END()