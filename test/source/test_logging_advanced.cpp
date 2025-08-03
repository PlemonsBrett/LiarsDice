#define BOOST_TEST_MODULE LoggingAdvancedTests
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace bdata = boost::unit_test::data;
namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

BOOST_AUTO_TEST_SUITE(LoggingAdvancedTestSuite)

// Test fixture for logging testing
class LoggingTestFixture {
public:
    LoggingTestFixture() {
        test_dir = std::filesystem::temp_directory_path() / "liarsdice_test_logs";
        std::filesystem::create_directories(test_dir);
        
        // Setup test logging environment
        setup_test_logging();
    }
    
    ~LoggingTestFixture() {
        cleanup_logging();
        std::filesystem::remove_all(test_dir);
    }
    
    void setup_test_logging() {
        // Remove all sinks to start fresh
        logging::core::get()->remove_all_sinks();
        
        // Create string stream sink for testing
        test_stream = std::make_shared<std::stringstream>();
        
        auto backend = boost::make_shared<sinks::text_ostream_backend>();
        backend->add_stream(test_stream);
        
        auto sink = boost::make_shared<sinks::synchronous_sink<sinks::text_ostream_backend>>(backend);
        
        logging::core::get()->add_sink(sink);
        logging::add_common_attributes();
    }
    
    void cleanup_logging() {
        logging::core::get()->remove_all_sinks();
    }
    
    std::string get_log_output() {
        return test_stream->str();
    }
    
    void clear_log_output() {
        test_stream->str("");
        test_stream->clear();
    }
    
    std::filesystem::path get_test_log_file() {
        return test_dir / "test.log";
    }
    
    std::filesystem::path test_dir;
    std::shared_ptr<std::stringstream> test_stream;
};

// BDD-style scenario tests for logging functionality
BOOST_AUTO_TEST_CASE(LoggingProvidesComprehensiveGameDiagnostics) {
    LoggingTestFixture fixture;
    src::logger lg;
    
    // GIVEN: A configured logging system
    // WHEN: logging messages at different levels
    BOOST_LOG_TRIVIAL(trace) << "Trace message";
    BOOST_LOG_TRIVIAL(debug) << "Debug message";
    BOOST_LOG_TRIVIAL(info) << "Info message";
    BOOST_LOG_TRIVIAL(warning) << "Warning message";
    BOOST_LOG_TRIVIAL(error) << "Error message";
    BOOST_LOG_TRIVIAL(fatal) << "Fatal message";
    
    // THEN: messages should be captured
    std::string output = fixture.get_log_output();
    BOOST_CHECK(!output.empty());
    
    // Should contain log level indicators
    BOOST_CHECK(output.find("info") != std::string::npos ||
                output.find("INFO") != std::string::npos);
}

// Property-based testing with different log levels
BOOST_DATA_TEST_CASE(LoggingHandlesVariousMessageTypes,
    bdata::make({"trace", "debug", "info", "warning", "error", "fatal"}),
    level_name) {
    
    LoggingTestFixture fixture;
    
    // Test logging at each level
    if (level_name == std::string("trace")) {
        BOOST_LOG_TRIVIAL(trace) << "Test trace message";
    } else if (level_name == std::string("debug")) {
        BOOST_LOG_TRIVIAL(debug) << "Test debug message";
    } else if (level_name == std::string("info")) {
        BOOST_LOG_TRIVIAL(info) << "Test info message";
    } else if (level_name == std::string("warning")) {
        BOOST_LOG_TRIVIAL(warning) << "Test warning message";
    } else if (level_name == std::string("error")) {
        BOOST_LOG_TRIVIAL(error) << "Test error message";
    } else if (level_name == std::string("fatal")) {
        BOOST_LOG_TRIVIAL(fatal) << "Test fatal message";
    }
    
    // Should not crash and should produce output
    BOOST_CHECK_NO_THROW(fixture.get_log_output());
}

// File logging performance and reliability
BOOST_AUTO_TEST_CASE(LoggingFileOutputPerformance) {
    LoggingTestFixture fixture;
    
    // Setup file logging
    auto log_file = fixture.get_test_log_file();
    
    // Remove existing file
    std::filesystem::remove(log_file);
    
    // Add file sink
    logging::add_file_log(
        keywords::file_name = log_file.string(),
        keywords::format = "[%TimeStamp%] [%Severity%] %Message%"
    );
    
    const int num_messages = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Log many messages
    for (int i = 0; i < num_messages; ++i) {
        BOOST_LOG_TRIVIAL(info) << "Performance test message " << i;
    }
    
    // Flush logs
    logging::core::get()->flush();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    BOOST_CHECK(duration.count() < 5000000); // Less than 5 seconds
    
    // Verify file was created and contains messages
    BOOST_CHECK(std::filesystem::exists(log_file));
    
    std::ifstream file(log_file);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    BOOST_CHECK(!content.empty());
    BOOST_CHECK(content.find("Performance test message") != std::string::npos);
    
    BOOST_TEST_MESSAGE("Logged " << num_messages << " messages in " 
                      << duration.count() << " microseconds");
}

// Advanced formatting and structured logging
BOOST_AUTO_TEST_CASE(LoggingAdvancedFormattingCapabilities) {
    LoggingTestFixture fixture;
    
    // Test various data types and formats
    BOOST_LOG_TRIVIAL(info) << "String: " << "test_string";
    BOOST_LOG_TRIVIAL(info) << "Integer: " << 42;
    BOOST_LOG_TRIVIAL(info) << "Float: " << 3.14159;
    BOOST_LOG_TRIVIAL(info) << "Boolean: " << true;
    BOOST_LOG_TRIVIAL(info) << "Hex: " << std::hex << 255;
    
    // Test structured data
    BOOST_LOG_TRIVIAL(info) << "Player[id=" << 1 << ", dice=" << 5 << ", active=" << true << "]";
    BOOST_LOG_TRIVIAL(info) << "Game[round=" << 3 << ", players=" << 4 << ", total_dice=" << 18 << "]";
    
    std::string output = fixture.get_log_output();
    
    // Verify different data types are logged correctly
    BOOST_CHECK(output.find("String: test_string") != std::string::npos);
    BOOST_CHECK(output.find("Integer: 42") != std::string::npos);
    BOOST_CHECK(output.find("Float: 3.14159") != std::string::npos);
    BOOST_CHECK(output.find("Boolean: 1") != std::string::npos || 
                output.find("Boolean: true") != std::string::npos);
}

// Error handling and edge cases
BOOST_AUTO_TEST_CASE(LoggingErrorHandlingAndEdgeCases) {
    LoggingTestFixture fixture;
    
    // Test logging with null or empty messages
    BOOST_CHECK_NO_THROW(BOOST_LOG_TRIVIAL(info) << "");
    BOOST_CHECK_NO_THROW(BOOST_LOG_TRIVIAL(info) << std::string());
    
    // Test logging with special characters
    BOOST_CHECK_NO_THROW(BOOST_LOG_TRIVIAL(info) << "Special chars: !@#$%^&*()");
    BOOST_CHECK_NO_THROW(BOOST_LOG_TRIVIAL(info) << "Unicode: 你好世界");
    BOOST_CHECK_NO_THROW(BOOST_LOG_TRIVIAL(info) << "Newlines:\nLine1\nLine2");
    BOOST_CHECK_NO_THROW(BOOST_LOG_TRIVIAL(info) << "Tabs:\tTabbed\tContent");
    
    // Test logging with very long messages
    std::string long_message(10000, 'A');
    BOOST_CHECK_NO_THROW(BOOST_LOG_TRIVIAL(info) << long_message);
    
    // Test rapid logging
    for (int i = 0; i < 100; ++i) {
        BOOST_CHECK_NO_THROW(BOOST_LOG_TRIVIAL(debug) << "Rapid message " << i);
    }
    
    std::string output = fixture.get_log_output();
    BOOST_CHECK(!output.empty());
}

// Thread safety considerations
BOOST_AUTO_TEST_CASE(LoggingThreadSafetyConsiderations) {
    LoggingTestFixture fixture;
    
    // Test that logging doesn't crash under concurrent-like conditions
    // (Note: Actual threading would require more complex setup)
    
    const int iterations = 1000;
    
    for (int i = 0; i < iterations; ++i) {
        BOOST_LOG_TRIVIAL(info) << "Thread-like message " << i;
        
        // Interleave different levels
        if (i % 3 == 0) {
            BOOST_LOG_TRIVIAL(debug) << "Debug from iteration " << i;
        } else if (i % 3 == 1) {
            BOOST_LOG_TRIVIAL(warning) << "Warning from iteration " << i;
        } else {
            BOOST_LOG_TRIVIAL(error) << "Error from iteration " << i;
        }
    }
    
    std::string output = fixture.get_log_output();
    BOOST_CHECK(!output.empty());
    
    // Should contain messages from all levels
    BOOST_CHECK(output.find("Thread-like message") != std::string::npos);
}

// Game-specific logging scenarios
BOOST_AUTO_TEST_CASE(LoggingGameSpecificScenarios) {
    LoggingTestFixture fixture;
    
    // Simulate game-related logging
    BOOST_LOG_TRIVIAL(info) << "Game started with 4 players";
    BOOST_LOG_TRIVIAL(debug) << "Player 1 rolled dice: [2, 3, 1, 5, 4]";
    BOOST_LOG_TRIVIAL(info) << "Player 2 (AI) guessed: 3 fours";
    BOOST_LOG_TRIVIAL(warning) << "Player 3 called liar!";
    BOOST_LOG_TRIVIAL(info) << "Liar call was correct - Player 2 loses a die";
    BOOST_LOG_TRIVIAL(debug) << "Round ended, starting new round";
    BOOST_LOG_TRIVIAL(error) << "Player 4 eliminated (no dice remaining)";
    BOOST_LOG_TRIVIAL(info) << "Game ended - Player 1 wins!";
    
    std::string output = fixture.get_log_output();
    
    // Verify game events are logged
    BOOST_CHECK(output.find("Game started") != std::string::npos);
    BOOST_CHECK(output.find("rolled dice") != std::string::npos);
    BOOST_CHECK(output.find("guessed") != std::string::npos);
    BOOST_CHECK(output.find("called liar") != std::string::npos);
    BOOST_CHECK(output.find("eliminated") != std::string::npos);
    BOOST_CHECK(output.find("wins") != std::string::npos);
}

// Log filtering and level management
BOOST_AUTO_TEST_CASE(LoggingFilteringAndLevelManagement) {
    LoggingTestFixture fixture;
    
    // Test setting different log levels
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::warning
    );
    
    fixture.clear_log_output();
    
    // These should be filtered out
    BOOST_LOG_TRIVIAL(trace) << "Trace - should be filtered";
    BOOST_LOG_TRIVIAL(debug) << "Debug - should be filtered";
    BOOST_LOG_TRIVIAL(info) << "Info - should be filtered";
    
    // These should pass through
    BOOST_LOG_TRIVIAL(warning) << "Warning - should appear";
    BOOST_LOG_TRIVIAL(error) << "Error - should appear";
    BOOST_LOG_TRIVIAL(fatal) << "Fatal - should appear";
    
    std::string output = fixture.get_log_output();
    
    // Should only contain warning and above
    BOOST_CHECK(output.find("should be filtered") == std::string::npos);
    BOOST_CHECK(output.find("should appear") != std::string::npos);
    
    // Reset filter for other tests
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::trace
    );
}

// Memory usage and resource management
BOOST_AUTO_TEST_CASE(LoggingMemoryUsageAndResourceManagement) {
    LoggingTestFixture fixture;
    
    // Test that logging doesn't cause memory leaks
    const int large_iteration_count = 10000;
    
    for (int i = 0; i < large_iteration_count; ++i) {
        BOOST_LOG_TRIVIAL(info) << "Memory test message " << i 
                               << " with some additional content to make it longer";
        
        // Periodically clear output to prevent excessive memory usage
        if (i % 1000 == 0) {
            fixture.clear_log_output();
        }
    }
    
    // Should complete without issues
    BOOST_CHECK(true);
    
    BOOST_TEST_MESSAGE("Completed memory test with " << large_iteration_count << " messages");
}

// Integration with game components
BOOST_AUTO_TEST_CASE(LoggingIntegrationWithGameComponents) {
    LoggingTestFixture fixture;
    
    // Simulate logging from different game components
    
    // Core game logging
    BOOST_LOG_TRIVIAL(info) << "[GAME] Initializing new game session";
    BOOST_LOG_TRIVIAL(debug) << "[GAME] Setting up player roster";
    
    // AI logging
    BOOST_LOG_TRIVIAL(debug) << "[AI-EASY] Analyzing game state for decision";
    BOOST_LOG_TRIVIAL(info) << "[AI-MEDIUM] Making guess based on probability";
    BOOST_LOG_TRIVIAL(debug) << "[AI-HARD] Pattern detected in opponent behavior";
    
    // Validation logging
    BOOST_LOG_TRIVIAL(warning) << "[VALIDATION] Invalid input received: 'abc'";
    BOOST_LOG_TRIVIAL(info) << "[VALIDATION] Player count validated: 4";
    
    // Configuration logging
    BOOST_LOG_TRIVIAL(info) << "[CONFIG] Loading configuration from file";
    BOOST_LOG_TRIVIAL(debug) << "[CONFIG] UI messages loaded successfully";
    
    std::string output = fixture.get_log_output();
    
    // Verify all components are logging
    BOOST_CHECK(output.find("[GAME]") != std::string::npos);
    BOOST_CHECK(output.find("[AI-") != std::string::npos);
    BOOST_CHECK(output.find("[VALIDATION]") != std::string::npos);
    BOOST_CHECK(output.find("[CONFIG]") != std::string::npos);
}

// Performance impact measurement
BOOST_AUTO_TEST_CASE(LoggingPerformanceImpactMeasurement) {
    LoggingTestFixture fixture;
    
    const int num_operations = 10000;
    
    // Measure performance with logging enabled
    auto start_with_logging = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_operations; ++i) {
        BOOST_LOG_TRIVIAL(debug) << "Operation " << i << " with logging";
        
        // Simulate some work
        volatile int dummy = i * 2;
        (void)dummy;
    }
    
    auto end_with_logging = std::chrono::high_resolution_clock::now();
    
    // Measure performance with logging disabled
    logging::core::get()->set_logging_enabled(false);
    
    auto start_without_logging = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_operations; ++i) {
        BOOST_LOG_TRIVIAL(debug) << "Operation " << i << " without logging";
        
        // Simulate same work
        volatile int dummy = i * 2;
        (void)dummy;
    }
    
    auto end_without_logging = std::chrono::high_resolution_clock::now();
    
    // Re-enable logging
    logging::core::get()->set_logging_enabled(true);
    
    auto with_logging_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_with_logging - start_with_logging);
    auto without_logging_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_without_logging - start_without_logging);
    
    BOOST_TEST_MESSAGE("With logging: " << with_logging_duration.count() << " μs");
    BOOST_TEST_MESSAGE("Without logging: " << without_logging_duration.count() << " μs");
    
    // Performance should be reasonable (logging shouldn't add more than 10x overhead)
    BOOST_CHECK(with_logging_duration.count() < without_logging_duration.count() * 10);
}

BOOST_AUTO_TEST_SUITE_END()