/**
 * @file logger_test.cpp
 * @brief Tests for the logging system
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#ifdef LIARSDICE_ENABLE_LOGGING
#include "liarsdice/logging/logging.hpp"
#include <memory>
#include <sstream>

using namespace liarsdice::logging;

// Test sink for capturing log output
class TestSink : public spdlog::sinks::base_sink<std::mutex> {
public:
  std::vector<std::string> messages;

protected:
  void sink_it_(const spdlog::details::log_msg &msg) override {
    spdlog::memory_buf_t formatted;
    base_sink<std::mutex>::formatter_->format(msg, formatted);
    messages.emplace_back(fmt::to_string(formatted));
  }

  void flush_() override {}
};

TEST_CASE("LoggerManager initialization", "[logging][manager]") {
  SECTION("Singleton instance works") {
    auto &manager1 = LoggerManager::instance();
    auto &manager2 = LoggerManager::instance();
    REQUIRE(&manager1 == &manager2);
  }

  SECTION("Default logger creation") {
    auto &manager = LoggerManager::instance();
    auto logger = manager.get_logger();
    REQUIRE(logger != nullptr);
  }

  SECTION("Component logger creation") {
    auto &manager = LoggerManager::instance();
    auto game_logger = manager.get_component_logger("game");
    auto dice_logger = manager.get_component_logger("dice");

    REQUIRE(game_logger != nullptr);
    REQUIRE(dice_logger != nullptr);
    REQUIRE(game_logger != dice_logger);
  }
}

TEST_CASE("Logging macros functionality", "[logging][macros]") {
  SECTION("Game logging macros work") {
    // These should compile and not crash
    GAME_LOG_INFO("Test game info message");
    GAME_LOG_DEBUG("Test game debug message with value: {}", 42);
    GAME_LOG_WARN("Test game warning");
    GAME_LOG_ERROR("Test game error");
  }

  SECTION("Dice logging macros work") {
    DICE_LOG_INFO("Test dice info message");
    DICE_LOG_DEBUG("Test dice debug with roll: {}", 6);
  }

  SECTION("Player logging macros work") {
    PLAYER_LOG_INFO("Test player info message");
    PLAYER_LOG_DEBUG("Test player debug for player {}", 1);
  }

  SECTION("Performance logging macros work") {
    PERF_LOG_INFO("Test performance message");
    // PERF_TIMER would need a more complex test setup
  }
}

TEST_CASE("Correlation ID functionality", "[logging][correlation]") {
  SECTION("Correlation ID generation") {
    auto id1 = NEW_CORRELATION_ID();
    auto id2 = NEW_CORRELATION_ID();

    REQUIRE(id1 != id2);
    REQUIRE(id1 > 0);
    REQUIRE(id2 > 0);
  }

  SECTION("Correlation scope RAII") {
    auto logger = get_default_logger();
    auto correlation_id = NEW_CORRELATION_ID();

    {
      WITH_CORRELATION_ID(*logger, correlation_id);
      // In a real implementation, we'd verify the correlation ID is set
      // For now, just ensure it compiles and doesn't crash
    }
    // Correlation ID should be cleared after scope
  }
}

TEST_CASE("Logger configuration", "[logging][config]") {
  SECTION("Logger initialization with config") {
    LoggerConfig config;
    config.environment = "test";
    config.async_logging = false; // Synchronous for testing

    // This should not throw
    REQUIRE_NOTHROW(LoggerManager::instance().initialize(config));
  }
}

TEST_CASE("Convenience logger getters", "[logging][convenience]") {
  SECTION("Get default logger") {
    auto logger = get_default_logger();
    REQUIRE(logger != nullptr);
  }

  SECTION("Get game logger") {
    auto logger = get_game_logger();
    REQUIRE(logger != nullptr);
  }

  SECTION("Get dice logger") {
    auto logger = get_dice_logger();
    REQUIRE(logger != nullptr);
  }

  SECTION("Get player logger") {
    auto logger = get_player_logger();
    REQUIRE(logger != nullptr);
  }

  SECTION("Get performance logger") {
    auto logger = get_performance_logger();
    REQUIRE(logger != nullptr);
  }
}

TEST_CASE("Logging system RAII", "[logging][raii]") {
  SECTION("LoggingSystem initialization and cleanup") {
    {
      LoggingSystem logging_system("test");
      // System should be initialized
      auto logger = get_default_logger();
      REQUIRE(logger != nullptr);
    }
    // System should be cleanly shut down
  }
}

TEST_CASE("Structured logging context", "[logging][structured]") {
  SECTION("LogContext construction") {
    LogContext context;
    context.component = "test";
    context.correlation_id = 12345;
    context.user_id = "test_user";
    context.session_id = "test_session";

    REQUIRE(context.component == "test");
    REQUIRE(context.correlation_id == 12345);
    REQUIRE(context.user_id == "test_user");
    REQUIRE(context.session_id == "test_session");
  }
}

#else

TEST_CASE("Logging disabled compilation", "[logging][disabled]") {
  SECTION("Macros compile to no-ops when logging disabled") {
    // These should all compile to no-ops
    GAME_LOG_INFO("This should be a no-op");
    DICE_LOG_DEBUG("This should be a no-op: {}", 42);
    PLAYER_LOG_WARN("This should be a no-op");
    PERF_LOG_INFO("This should be a no-op");
    PERF_TIMER("test_operation");

    auto id = NEW_CORRELATION_ID();
    REQUIRE(id == 0ULL); // Should return 0 when disabled
  }
}

#endif // LIARSDICE_ENABLE_LOGGING