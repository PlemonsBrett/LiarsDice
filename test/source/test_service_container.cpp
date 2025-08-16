#define BOOST_TEST_MODULE ServiceContainerTests
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>
#include <liarsdice/di/service_container.hpp>

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

class AnotherTestService : public ITestService {
public:
  std::string get_name() const override { return "AnotherTestService"; }
  int get_value() const override { return 100; }
};

// Test suite for basic container functionality
BOOST_AUTO_TEST_SUITE(ServiceContainerBasicTests)

BOOST_AUTO_TEST_CASE(container_registration_transient) {
  ServiceContainer container;

  container.register_service<ITestService, TestServiceImpl>();

  BOOST_CHECK(container.is_registered<ITestService>());
  BOOST_CHECK_EQUAL(container.service_count(), 1);
}

BOOST_AUTO_TEST_CASE(container_registration_singleton) {
  ServiceContainer container;

  container.register_service<ITestService, TestServiceImpl>(ServiceLifetime::Singleton);

  auto service1 = container.resolve<ITestService>();
  auto service2 = container.resolve<ITestService>();

  // Should be the same instance for singleton
  BOOST_CHECK_EQUAL(service1.get(), service2.get());
  BOOST_CHECK_EQUAL(service1->get_name(), "TestService");
}

BOOST_AUTO_TEST_CASE(container_resolution_transient) {
  ServiceContainer container;

  container.register_service<ITestService, TestServiceImpl>();

  auto service1 = container.resolve<ITestService>();
  auto service2 = container.resolve<ITestService>();

  // Should be different instances for transient
  BOOST_CHECK_NE(service1.get(), service2.get());
  BOOST_CHECK_EQUAL(service1->get_name(), "TestService");
  BOOST_CHECK_EQUAL(service2->get_value(), 42);
}

BOOST_AUTO_TEST_CASE(container_instance_registration) {
  ServiceContainer container;

  auto instance = std::make_shared<TestServiceImpl>();
  container.register_instance<ITestService>(instance);

  auto resolved = container.resolve<ITestService>();

  BOOST_CHECK_EQUAL(instance.get(), resolved.get());
}

BOOST_AUTO_TEST_CASE(container_unregistered_service_throws) {
  ServiceContainer container;

  BOOST_CHECK_THROW(container.resolve<ITestService>(), ServiceContainerException);
}

BOOST_AUTO_TEST_SUITE_END()

// Test suite for advanced functionality
BOOST_AUTO_TEST_SUITE(ServiceContainerAdvancedTests)

BOOST_AUTO_TEST_CASE(container_custom_factory) {
  ServiceContainer container;

  container.register_factory<ITestService>([]() { return std::make_shared<AnotherTestService>(); });

  auto service = container.resolve<ITestService>();
  BOOST_CHECK_EQUAL(service->get_name(), "AnotherTestService");
  BOOST_CHECK_EQUAL(service->get_value(), 100);
}

BOOST_AUTO_TEST_CASE(container_clear_services) {
  ServiceContainer container;

  container.register_service<ITestService, TestServiceImpl>();
  BOOST_CHECK_EQUAL(container.service_count(), 1);

  container.clear();
  BOOST_CHECK_EQUAL(container.service_count(), 0);
  BOOST_CHECK(!container.is_registered<ITestService>());
}

BOOST_AUTO_TEST_CASE(global_container_access) {
  auto& container = get_service_container();

  container.clear();  // Start clean
  container.register_service<ITestService, TestServiceImpl>();

  BOOST_CHECK(container.is_registered<ITestService>());

  // Test that it's the same instance
  auto& container2 = get_service_container();
  BOOST_CHECK(container2.is_registered<ITestService>());

  container.clear();  // Clean up
}

BOOST_AUTO_TEST_SUITE_END()

// Data-driven test example (Boost.Test feature)
namespace bdata = boost::unit_test::data;

BOOST_DATA_TEST_CASE(service_lifetime_test,
                     bdata::make({ServiceLifetime::Transient, ServiceLifetime::Singleton}),
                     lifetime) {
  ServiceContainer container;
  container.register_service<ITestService, TestServiceImpl>(lifetime);

  auto service1 = container.resolve<ITestService>();
  auto service2 = container.resolve<ITestService>();

  if (lifetime == ServiceLifetime::Singleton) {
    BOOST_CHECK_EQUAL(service1.get(), service2.get());
  } else {
    BOOST_CHECK_NE(service1.get(), service2.get());
  }
}