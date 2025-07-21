/**
 * @file config_manager_test.cpp
 * @brief Tests for the configuration manager system
 */

#include <catch2/catch_test_macros.hpp>

#ifdef LIARSDICE_ENABLE_CONFIG
#include "liarsdice/config/config_manager.hpp"
#include "liarsdice/config/config_sources.hpp"
#include <memory>

using namespace liarsdice::config;

// Test configuration source for testing
class TestConfigSource : public IConfigSource {
public:
    TestConfigSource(int priority = 100) : priority_(priority) {}
    
    void add_value(const ConfigPath& path, const std::string& value) {
        data_[path.to_string()] = value;
    }
    
    bool has_value(const ConfigPath& path) const override {
        return data_.contains(path.to_string());
    }
    
    std::optional<std::string> get_raw_value(const ConfigPath& path) const override {
        auto it = data_.find(path.to_string());
        return (it != data_.end()) ? std::optional<std::string>{it->second} : std::nullopt;
    }
    
    std::vector<ConfigPath> get_all_paths() const override {
        std::vector<ConfigPath> paths;
        for (const auto& [key, _] : data_) {
            paths.emplace_back(key);
        }
        return paths;
    }
    
    int get_priority() const noexcept override { return priority_; }
    std::string get_name() const override { return "TestSource"; }

private:
    std::unordered_map<std::string, std::string> data_;
    int priority_;
};

TEST_CASE("ConfigManager basic operations", "[config][manager]") {
    ConfigManager config;
    
    SECTION("Empty manager returns no values") {
        REQUIRE_FALSE(config.has(ConfigPath{"nonexistent"}));
        REQUIRE_FALSE(config.get<std::string>(ConfigPath{"nonexistent"}).has_value());
        REQUIRE(config.get_or<int>(ConfigPath{"nonexistent"}, 42) == 42);
        REQUIRE_THROWS_AS(config.get_required<int>(ConfigPath{"nonexistent"}), ConfigException);
    }
    
    SECTION("Single source provides values") {
        auto source = std::make_unique<TestConfigSource>();
        source->add_value(ConfigPath{"test.key"}, "test_value");
        source->add_value(ConfigPath{"test.number"}, "42");
        config.add_source(std::move(source));
        
        REQUIRE(config.has(ConfigPath{"test.key"}));
        REQUIRE(config.get<std::string>(ConfigPath{"test.key"}) == "test_value");
        REQUIRE(config.get<int32_t>(ConfigPath{"test.number"}) == 42);
        REQUIRE_FALSE(config.has(ConfigPath{"nonexistent"}));
    }
}

TEST_CASE("ConfigManager source priority", "[config][manager][priority]") {
    ConfigManager config;
    
    auto low_priority = std::make_unique<TestConfigSource>(50);
    low_priority->add_value(ConfigPath{"test.key"}, "low_value");
    
    auto high_priority = std::make_unique<TestConfigSource>(150);
    high_priority->add_value(ConfigPath{"test.key"}, "high_value");
    
    config.add_source(std::move(low_priority));
    config.add_source(std::move(high_priority));
    
    // High priority source should override low priority
    REQUIRE(config.get<std::string>(ConfigPath{"test.key"}) == "high_value");
}

TEST_CASE("ConfigManager runtime overrides", "[config][manager][override]") {
    ConfigManager config;
    
    auto source = std::make_unique<TestConfigSource>();
    source->add_value(ConfigPath{"test.key"}, "source_value");
    config.add_source(std::move(source));
    
    // Source value is returned initially
    REQUIRE(config.get<std::string>(ConfigPath{"test.key"}) == "source_value");
    
    // Runtime override takes precedence
    config.set<std::string>(ConfigPath{"test.key"}, "override_value");
    REQUIRE(config.get<std::string>(ConfigPath{"test.key"}) == "override_value");
}

TEST_CASE("ConfigManager change listeners", "[config][manager][listeners]") {
    ConfigManager config;
    
    bool change_detected = false;
    ConfigPath changed_path;
    std::string old_value, new_value;
    
    config.add_change_listener("test_listener", [&](const ConfigChangeEvent& event) {
        change_detected = true;
        changed_path = event.path;
        if (auto old_str = event.old_value.get<std::string>()) {
            old_value = *old_str;
        }
        if (auto new_str = event.new_value.get<std::string>()) {
            new_value = *new_str;
        }
    });
    
    config.set<std::string>(ConfigPath{"test.key"}, "new_value");
    
    REQUIRE(change_detected);
    REQUIRE(changed_path.to_string() == "test.key");
    REQUIRE(new_value == "new_value");
}

TEST_CASE("ConfigManager get_all_paths", "[config][manager][paths]") {
    ConfigManager config;
    
    auto source1 = std::make_unique<TestConfigSource>();
    source1->add_value(ConfigPath{"source1.key1"}, "value1");
    source1->add_value(ConfigPath{"source1.key2"}, "value2");
    
    auto source2 = std::make_unique<TestConfigSource>();
    source2->add_value(ConfigPath{"source2.key1"}, "value3");
    
    config.add_source(std::move(source1));
    config.add_source(std::move(source2));
    
    // Add runtime override
    config.set<std::string>(ConfigPath{"runtime.key"}, "runtime_value");
    
    auto all_paths = config.get_all_paths();
    REQUIRE(all_paths.size() == 4); // 3 from sources + 1 runtime override
    
    // Check that all expected paths are present
    std::set<std::string> path_strings;
    for (const auto& path : all_paths) {
        path_strings.insert(path.to_string());
    }
    
    REQUIRE(path_strings.contains("source1.key1"));
    REQUIRE(path_strings.contains("source1.key2"));
    REQUIRE(path_strings.contains("source2.key1"));
    REQUIRE(path_strings.contains("runtime.key"));
}

TEST_CASE("ConfigManager get_section", "[config][manager][section]") {
    ConfigManager config;
    
    auto source = std::make_unique<TestConfigSource>();
    source->add_value(ConfigPath{"game.rules.max_players"}, "6");
    source->add_value(ConfigPath{"game.rules.dice_per_player"}, "5");
    source->add_value(ConfigPath{"game.ui.theme"}, "dark");
    source->add_value(ConfigPath{"network.port"}, "7777");
    
    config.add_source(std::move(source));
    
    auto game_section = config.get_section(ConfigPath{"game"});
    REQUIRE(game_section.size() == 2); // rules and ui are immediate children
    
    auto rules_section = config.get_section(ConfigPath{"game.rules"});
    REQUIRE(rules_section.size() == 2); // max_players and dice_per_player
    REQUIRE(rules_section.contains("max_players"));
    REQUIRE(rules_section.contains("dice_per_player"));
}

#endif // LIARSDICE_ENABLE_CONFIG