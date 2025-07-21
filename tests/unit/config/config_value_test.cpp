/**
 * @file config_value_test.cpp
 * @brief Tests for the configuration value system
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#ifdef LIARSDICE_ENABLE_CONFIG
#include "liarsdice/config/config_value.hpp"

using namespace liarsdice::config;

TEST_CASE("ConfigValue basic operations", "[config][value]") {
  SECTION("Default construction creates unset value") {
    ConfigValue value;
    REQUIRE_FALSE(value.is_set());
    REQUIRE(value.type_index() == 0); // monostate
  }

  SECTION("Construction from supported types") {
    ConfigValue bool_val{true};
    REQUIRE(bool_val.is_set());
    REQUIRE(bool_val.is_type<bool>());
    REQUIRE(bool_val.get<bool>() == true);

    ConfigValue int_val{42};
    REQUIRE(int_val.is_set());
    REQUIRE(int_val.is_type<int32_t>());
    REQUIRE(int_val.get<int32_t>() == 42);

    ConfigValue string_val{std::string{"test"}};
    REQUIRE(string_val.is_set());
    REQUIRE(string_val.is_type<std::string>());
    REQUIRE(string_val.get<std::string>() == "test");
  }

  SECTION("get_or returns default for unset values") {
    ConfigValue value;
    REQUIRE(value.get_or<int32_t>(123) == 123);
    REQUIRE(value.get_or<std::string>("default") == "default");
  }

  SECTION("get_or returns stored value for set values") {
    ConfigValue value{42};
    REQUIRE(value.get_or<int32_t>(123) == 42);
  }

  SECTION("get_required throws for unset values") {
    ConfigValue value;
    REQUIRE_THROWS_AS(value.get_required<int32_t>(), ConfigException);
  }

  SECTION("Type safety prevents wrong type access") {
    ConfigValue value{42};
    REQUIRE_FALSE(value.get<std::string>().has_value());
    REQUIRE_FALSE(value.is_type<std::string>());
  }
}

TEST_CASE("ConfigValue string conversion", "[config][value][string]") {
  SECTION("to_string works for all types") {
    REQUIRE(ConfigValue{}.to_string() == "<unset>");
    REQUIRE(ConfigValue{true}.to_string() == "true");
    REQUIRE(ConfigValue{false}.to_string() == "false");
    REQUIRE(ConfigValue{42}.to_string() == "42");
    REQUIRE(ConfigValue{3.14}.to_string() == "3.140000");
    REQUIRE(ConfigValue{std::string{"hello"}}.to_string() == "hello");

    std::vector<std::string> vec{"a", "b", "c"};
    ConfigValue vec_val{vec};
    REQUIRE(vec_val.to_string() == "[\"a\", \"b\", \"c\"]");
  }

  SECTION("from_string parses basic types") {
    auto bool_val = ConfigValue::from_string("true", 1);
    REQUIRE(bool_val.has_value());
    REQUIRE(bool_val->get<bool>() == true);

    auto int_val = ConfigValue::from_string("42", 2);
    REQUIRE(int_val.has_value());
    REQUIRE(int_val->get<int32_t>() == 42);

    auto double_val = ConfigValue::from_string("3.14", 6);
    REQUIRE(double_val.has_value());
    REQUIRE(double_val->get<double>() == Catch::Approx(3.14));

    auto string_val = ConfigValue::from_string("hello", 7);
    REQUIRE(string_val.has_value());
    REQUIRE(string_val->get<std::string>() == "hello");
  }

  SECTION("from_string handles invalid input gracefully") {
    auto invalid_int = ConfigValue::from_string("not_a_number", 2);
    REQUIRE_FALSE(invalid_int.has_value());

    auto invalid_bool = ConfigValue::from_string("maybe", 1);
    REQUIRE_FALSE(invalid_bool.has_value());
  }
}

TEST_CASE("ConfigPath operations", "[config][path]") {
  SECTION("Construction from string") {
    ConfigPath empty_path;
    REQUIRE(empty_path.is_root());
    REQUIRE(empty_path.to_string() == "");

    ConfigPath simple_path{"key"};
    REQUIRE_FALSE(simple_path.is_root());
    REQUIRE(simple_path.to_string() == "key");
    REQUIRE(simple_path.segments().size() == 1);
    REQUIRE(simple_path.segments()[0] == "key");

    ConfigPath nested_path{"game.rules.max_players"};
    REQUIRE(nested_path.segments().size() == 3);
    REQUIRE(nested_path.segments()[0] == "game");
    REQUIRE(nested_path.segments()[1] == "rules");
    REQUIRE(nested_path.segments()[2] == "max_players");
  }

  SECTION("Parent and append operations") {
    ConfigPath path{"game.rules.max_players"};

    auto parent = path.parent();
    REQUIRE(parent.has_value());
    REQUIRE(parent->to_string() == "game.rules");

    auto grandparent = parent->parent();
    REQUIRE(grandparent.has_value());
    REQUIRE(grandparent->to_string() == "game");

    auto root = grandparent->parent();
    REQUIRE(root.has_value());
    REQUIRE(root->is_root());

    auto no_parent = root->parent();
    REQUIRE_FALSE(no_parent.has_value());

    auto extended = path.append("timeout");
    REQUIRE(extended.to_string() == "game.rules.max_players.timeout");
  }

  SECTION("Comparison operators work") {
    ConfigPath path1{"game.rules"};
    ConfigPath path2{"game.rules"};
    ConfigPath path3{"game.ui"};

    REQUIRE(path1 == path2);
    REQUIRE(path1 != path3);
    REQUIRE(path1 < path3); // "rules" < "ui"
  }
}

TEST_CASE("ConfigValidator operations", "[config][validator]") {
  SECTION("Range validator works") {
    auto validator = make_range_validator<int>(0, 100);

    REQUIRE(validator.validate(50));
    REQUIRE(validator.validate(0));
    REQUIRE(validator.validate(100));
    REQUIRE_FALSE(validator.validate(-1));
    REQUIRE_FALSE(validator.validate(101));

    REQUIRE(validator.get_error_message().empty()); // Last validation was successful

    validator.validate(-1);
    REQUIRE_FALSE(validator.get_error_message().empty());
  }

  SECTION("Custom validator works") {
    ConfigValidator<std::string> validator(
        [](const std::string &value) { return value.length() >= 3; },
        []() { return "String must be at least 3 characters"; });

    REQUIRE(validator.validate("hello"));
    REQUIRE(validator.validate("abc"));
    REQUIRE_FALSE(validator.validate("ab"));
    REQUIRE_FALSE(validator.validate(""));

    validator.validate("x");
    REQUIRE(validator.get_error_message() == "String must be at least 3 characters");
  }
}

#endif // LIARSDICE_ENABLE_CONFIG