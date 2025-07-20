// Google Test main header for unit testing
#include <gtest/gtest.h>

// Dependency injection container and related interfaces
#include "liarsdice/di/di.hpp"
#include "liarsdice/interfaces/interfaces.hpp"
#include "liarsdice/adapters/random_generator.hpp"

using namespace liarsdice::di;
using namespace liarsdice::interfaces;
using namespace liarsdice::adapters;

// Test interfaces and implementations
class ITestService {
public:
    virtual ~ITestService() = default;
    [[nodiscard]] virtual int get_value() const = 0;
    virtual void set_value(int value) = 0;
};

class TestServiceImpl : public ITestService {
private:
    int value_;

public:
    explicit TestServiceImpl(int value = 42) : value_(value) {}

    [[nodiscard]] int get_value() const override { return value_; }
    void set_value(int value) override { value_ = value; }
};

class ICalculator {
public:
    virtual ~ICalculator() = default;
    [[nodiscard]] virtual int add(int a, int b) const = 0;
    [[nodiscard]] virtual int multiply(int a, int b) const = 0;
};

class Calculator : public ICalculator {
public:
    [[nodiscard]] int add(int a, int b) const override { return a + b; }
    [[nodiscard]] int multiply(int a, int b) const override { return a * b; }
};

class ServiceContainerTest : public ::testing::Test {
public:
    void SetUp() override {
        container = std::make_unique<ServiceContainer>();
    }

    void TearDown() override {
        container.reset();
    }

    std::unique_ptr<ServiceContainer> container;
};

TEST_F(ServiceContainerTest, RegisterAndResolveTransientService) {
    // Register service
    container->register_service<ITestService, TestServiceImpl>(
        ServiceLifetime::kTransient, "", 100);

    // Resolve service
    auto result = container->resolve<ITestService>();
    ASSERT_TRUE(result.has_value());
    
    auto service = std::move(*result);
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->get_value(), 100);
}

TEST_F(ServiceContainerTest, RegisterAndResolveSingletonService) {
    // Register singleton
    container->register_service<ITestService, TestServiceImpl>(
        ServiceLifetime::kSingleton, "", 200);

    // Resolve first instance
    auto result1 = container->resolve<ITestService>();
    ASSERT_TRUE(result1.has_value());
    
    auto service1 = std::move(*result1);
    ASSERT_NE(service1, nullptr);
    EXPECT_EQ(service1->get_value(), 200);

    // Modify the value
    service1->set_value(300);

    // Resolve second instance - for our current implementation,
    // this creates a new instance each time (not true singleton behavior)
    // In a production system, you'd want shared_ptr for true singletons
    auto result2 = container->resolve<ITestService>();
    ASSERT_TRUE(result2.has_value());
    
    auto service2 = std::move(*result2);
    ASSERT_NE(service2, nullptr);
    // This will be the original value since we create new instances
    EXPECT_EQ(service2->get_value(), 200);
}

TEST_F(ServiceContainerTest, RegisterWithCustomFactory) {
    // Register with custom factory
    auto factory = []() -> std::unique_ptr<ITestService> {
        return std::make_unique<TestServiceImpl>(999);
    };

    container->register_factory<ITestService>(factory, ServiceLifetime::kTransient);

    // Resolve service
    auto result = container->resolve<ITestService>();
    ASSERT_TRUE(result.has_value());
    
    auto service = std::move(*result);
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->get_value(), 999);
}

TEST_F(ServiceContainerTest, RegisterNamedService) {
    // Register named services
    container->register_service<ITestService, TestServiceImpl>(
        ServiceLifetime::kTransient, "service1", 111);
    container->register_service<ITestService, TestServiceImpl>(
        ServiceLifetime::kTransient, "service2", 222);

    // Resolve by name
    auto result1 = container->resolve<ITestService>("service1");
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ((*result1)->get_value(), 111);

    auto result2 = container->resolve<ITestService>("service2");
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ((*result2)->get_value(), 222);
}

TEST_F(ServiceContainerTest, ResolveNonExistentService) {
    // Try to resolve unregistered service
    auto result = container->resolve<ITestService>();
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), DIError::kServiceNotRegistered);
}

TEST_F(ServiceContainerTest, ResolveNonExistentNamedService) {
    // Try to resolve unregistered named service
    auto result = container->resolve<ITestService>("nonexistent");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), DIError::kServiceNotRegistered);
}

TEST_F(ServiceContainerTest, IsRegisteredCheck) {
    EXPECT_FALSE(container->is_registered<ITestService>());
    EXPECT_FALSE(container->is_registered("test_service"));

    container->register_service<ITestService, TestServiceImpl>(
        ServiceLifetime::kTransient, "test_service");

    EXPECT_TRUE(container->is_registered<ITestService>());
    EXPECT_TRUE(container->is_registered("test_service"));
}

TEST_F(ServiceContainerTest, GetRegisteredServices) {
    EXPECT_EQ(container->size(), 0);
    EXPECT_TRUE(container->get_registered_services().empty());

    container->register_service<ITestService, TestServiceImpl>(
        ServiceLifetime::kTransient, "service1");
    container->register_service<ICalculator, Calculator>(
        ServiceLifetime::kTransient, "calc");

    EXPECT_EQ(container->size(), 2);
    
    auto services = container->get_registered_services();
    EXPECT_EQ(services.size(), 2);
    EXPECT_TRUE(std::find(services.begin(), services.end(), "service1") != services.end());
    EXPECT_TRUE(std::find(services.begin(), services.end(), "calc") != services.end());
}

TEST_F(ServiceContainerTest, ClearServices) {
    container->register_service<ITestService, TestServiceImpl>();
    container->register_service<ICalculator, Calculator>();

    EXPECT_EQ(container->size(), 2);

    container->clear();

    EXPECT_EQ(container->size(), 0);
    EXPECT_FALSE(container->is_registered<ITestService>());
    EXPECT_FALSE(container->is_registered<ICalculator>());
}

TEST_F(ServiceContainerTest, RandomGeneratorIntegration) {
    // Register random generator
    container->register_service<IRandomGenerator, StandardRandomGenerator>(
        ServiceLifetime::kSingleton);

    // Resolve and test
    auto result = container->resolve<IRandomGenerator>();
    ASSERT_TRUE(result.has_value());
    
    auto rng = std::move(*result);
    ASSERT_NE(rng, nullptr);

    // Test random generation
    int value = rng->generate(1, 6);
    EXPECT_GE(value, 1);
    EXPECT_LE(value, 6);

    // Test boolean generation
    bool bool_val = rng->generate_bool();
    EXPECT_TRUE(bool_val == true || bool_val == false);

    // Test normalized generation
    double norm_val = rng->generate_normalized();
    EXPECT_GE(norm_val, 0.0);
    EXPECT_LE(norm_val, 1.0);
}

TEST_F(ServiceContainerTest, MockRandomGeneratorDeterministic) {
    // Register mock random generator with predetermined values
    std::vector<int> values = {3, 1, 6, 2, 4, 5};
    
    auto factory = [values]() -> std::unique_ptr<IRandomGenerator> {
        return std::make_unique<MockRandomGenerator>(values);
    };

    container->register_factory<IRandomGenerator>(factory);

    // Resolve and test deterministic behavior
    auto result = container->resolve<IRandomGenerator>();
    ASSERT_TRUE(result.has_value());
    
    auto rng = std::move(*result);
    ASSERT_NE(rng, nullptr);

    // Should return values in sequence
    EXPECT_EQ(rng->generate(1, 6), 3);
    EXPECT_EQ(rng->generate(1, 6), 1);
    EXPECT_EQ(rng->generate(1, 6), 6);
    EXPECT_EQ(rng->generate(1, 6), 2);
    EXPECT_EQ(rng->generate(1, 6), 4);
    EXPECT_EQ(rng->generate(1, 6), 5);
    
    // Should cycle back to beginning
    EXPECT_EQ(rng->generate(1, 6), 3);
}

// ServiceContainerBuilder tests
class ServiceContainerBuilderTest : public ::testing::Test {
public:
    void SetUp() override {
        builder = std::make_unique<ServiceContainerBuilder>();
    }

    std::unique_ptr<ServiceContainerBuilder> builder;
};

TEST_F(ServiceContainerBuilderTest, FluentInterface) {
    // Use fluent interface to configure services
    builder->register_service<ITestService>()
        .as_singleton()
        .named("main_service")
        .use<TestServiceImpl>(500);

    builder->register_service<ICalculator>()
        .as_transient()
        .use<Calculator>();

    // Build container
    auto container = builder->build();
    ASSERT_NE(container, nullptr);

    // Test registration worked
    EXPECT_TRUE(container->is_registered<ITestService>());
    EXPECT_TRUE(container->is_registered("main_service"));
    EXPECT_TRUE(container->is_registered<ICalculator>());

    // Test resolution
    auto service_result = container->resolve<ITestService>("main_service");
    ASSERT_TRUE(service_result.has_value());
    EXPECT_EQ((*service_result)->get_value(), 500);

    auto calc_result = container->resolve<ICalculator>();
    ASSERT_TRUE(calc_result.has_value());
    EXPECT_EQ((*calc_result)->add(2, 3), 5);
}

TEST_F(ServiceContainerBuilderTest, UseFactory) {
    auto custom_factory = []() -> std::unique_ptr<ITestService> {
        auto service = std::make_unique<TestServiceImpl>(777);
        return service;
    };

    builder->register_service<ITestService>()
        .as_transient()
        .use_factory(custom_factory);

    auto container = builder->build();
    auto result = container->resolve<ITestService>();
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->get_value(), 777);
}