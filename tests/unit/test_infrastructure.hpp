#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include "liarsdice/interfaces/i_dice.hpp"
#include "liarsdice/interfaces/i_player.hpp"
#include "liarsdice/interfaces/i_game.hpp"
#include "liarsdice/interfaces/i_random_generator.hpp"
#include "liarsdice/di/service_container.hpp"

#include <concepts>
#include <ranges>
#include <memory>
#include <random>
#include <numeric>
#include <sstream>
#include <chrono>

namespace liarsdice::testing {

// Modern C++23 concepts for test constraints
template<typename T>
concept DiceTestable = requires(T t) {
    { t.get_face_value() } -> std::same_as<unsigned int>;
    { t.roll() } -> std::same_as<void>;
    { t.is_valid_face_value(1u) } -> std::same_as<bool>;
};

template<typename T>
concept PlayerTestable = requires(T t) {
    { t.get_id() } -> std::same_as<int>;
    { t.get_dice_count() } -> std::same_as<size_t>;
    { t.has_dice() } -> std::same_as<bool>;
};

// Custom Catch2 matchers leveraging C++23 concepts and ranges
template<std::ranges::range Range>
struct InRangeMatcher : Catch::Matchers::MatcherGenericBase {
    Range expected_range;
    
    explicit InRangeMatcher(Range range) : expected_range(std::move(range)) {}
    
    template<typename T>
    bool match(const T& value) const {
        return std::ranges::find(expected_range, value) != std::ranges::end(expected_range);
    }
    
    std::string describe() const override {
        std::ostringstream oss;
        oss << "is in range [";
        bool first = true;
        for (const auto& val : expected_range) {
            if (!first) oss << ", ";
            oss << val;
            first = false;
        }
        oss << "]";
        return oss.str();
    }
};

template<std::ranges::range Range>
auto IsInRange(Range&& range) {
    return InRangeMatcher{std::forward<Range>(range)};
}

// Property-based testing data generator helpers
template<typename T>
    requires std::integral<T>
std::vector<T> generate_integers(T min, T max, size_t count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<T> dist(min, max);
    
    std::vector<T> result;
    result.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        result.push_back(dist(gen));
    }
    
    return result;
}

// Test data builder with perfect forwarding and RAII
template<typename T>
class TestDataBuilder {
private:
    std::unique_ptr<T> object_;
    
public:
    TestDataBuilder() : object_(std::make_unique<T>()) {}
    
    template<typename... Args>
    TestDataBuilder& with(Args&&... args) {
        std::apply([this](auto&&... a) {
            ((void)this->apply_property(std::forward<decltype(a)>(a)), ...);
        }, std::forward_as_tuple(args...));
        return *this;
    }
    
    std::unique_ptr<T> build() && {
        return std::move(object_);
    }
    
private:
    template<typename Prop>
    void apply_property(Prop&& prop) {
        prop(*object_);
    }
};

// Mock implementation for testing
class MockRandomGenerator : public interfaces::IRandomGenerator {
private:
    mutable std::vector<int> sequence_;
    mutable size_t index_ = 0;
    mutable std::mt19937 fallback_gen_{std::random_device{}()};
    
public:
    explicit MockRandomGenerator(std::vector<int> sequence = {})
        : sequence_(std::move(sequence)) {}
    
    int generate(int min, int max) override {
        if (index_ < sequence_.size()) {
            return sequence_[index_++];
        }
        std::uniform_int_distribution<int> dist(min, max);
        return dist(fallback_gen_);
    }
    
    void seed(unsigned int seed) override {
        fallback_gen_.seed(seed);
        index_ = 0;
    }
    
    bool generate_bool() override {
        return generate(0, 1) == 1;
    }
    
    double generate_normalized() override {
        return static_cast<double>(generate(0, RAND_MAX)) / RAND_MAX;
    }
    
    void set_sequence(std::vector<int> sequence) {
        sequence_ = std::move(sequence);
        index_ = 0;
    }
};

// Test fixture base class using Catch2's TEST_CASE_METHOD
class GameTestFixture {
protected:
    std::unique_ptr<liarsdice::di::ServiceContainer> container_;
    std::unique_ptr<interfaces::IGame> game_;
    
    GameTestFixture() {
        container_ = std::make_unique<liarsdice::di::ServiceContainer>();
        setup_dependencies();
    }
    
    virtual ~GameTestFixture() = default;
    
    virtual void setup_dependencies() {
        // Register test dependencies
        container_->register_service<interfaces::IRandomGenerator, MockRandomGenerator>(
            liarsdice::di::ServiceLifetime::kSingleton
        );
    }
    
    void create_game_with_players(size_t player_count) {
        game_ = container_->resolve<interfaces::IGame>().value();
        game_->initialize();
        
        for (size_t i = 1; i <= player_count; ++i) {
            game_->add_player(static_cast<int>(i));
        }
        
        game_->start_game();
    }
};

// BDD-style test helpers are already provided by Catch2

// Performance benchmark helper
template<typename Func>
auto benchmark_with_stats(const std::string& name, Func&& func, size_t iterations = 1000) {
    using namespace std::chrono;
    
    std::vector<double> timings;
    timings.reserve(iterations);
    
    for (size_t i = 0; i < iterations; ++i) {
        auto start = high_resolution_clock::now();
        func();
        auto end = high_resolution_clock::now();
        
        duration<double, std::micro> elapsed = end - start;
        timings.push_back(elapsed.count());
    }
    
    // Calculate statistics
    std::ranges::sort(timings);
    double median = timings[timings.size() / 2];
    double mean = std::reduce(timings.begin(), timings.end()) / timings.size();
    double p95 = timings[static_cast<size_t>(timings.size() * 0.95)];
    
    return std::make_tuple(mean, median, p95);
}

} // namespace liarsdice::testing