#pragma once

#include <concepts>
#include <type_traits>
#include <memory>

namespace liarsdice::interfaces {

// Forward declarations for concepts
// (These are just for concept definitions, not actual classes)

// Concept for dice interface
template<typename T>
concept DiceInterface = requires(T dice) {
    { dice.roll() } -> std::same_as<void>;
    { dice.get_face_value() } -> std::convertible_to<unsigned int>;
    { dice.set_face_value(1U) } -> std::same_as<void>;
    { dice.is_valid_face_value(1U) } -> std::same_as<bool>;
};

// Concept for player interface
template<typename T>
concept PlayerInterface = requires(T player) {
    { player.get_id() } -> std::convertible_to<int>;
    { player.get_dice_count() } -> std::convertible_to<size_t>;
    { player.roll_dice() } -> std::same_as<void>;
    { player.add_die() } -> std::same_as<void>;
    { player.remove_die() } -> std::same_as<bool>;
    { player.has_dice() } -> std::same_as<bool>;
};

// Concept for game state interface
template<typename T>
concept GameStateInterface = requires(T state) {
    { state.get_current_player_index() } -> std::convertible_to<size_t>;
    { state.get_player_count() } -> std::convertible_to<size_t>;
    { state.is_game_active() } -> std::same_as<bool>;
    { state.get_round_number() } -> std::convertible_to<int>;
};

// Concept for game interface
template<typename T>
concept GameInterface = requires(T game) {
    { game.initialize() } -> std::same_as<void>;
    { game.add_player(1) } -> std::same_as<void>;
    { game.start_game() } -> std::same_as<void>;
    { game.is_game_over() } -> std::same_as<bool>;
    { game.get_winner_id() } -> std::convertible_to<int>;
};

// Concept for random number generator interface
template<typename T>
concept RandomGeneratorInterface = requires(T rng) {
    { rng.generate(1, 6) } -> std::convertible_to<int>;
    { rng.seed(0U) } -> std::same_as<void>;
};

// Concept for service interface (for DI container)
template<typename T>
concept ServiceInterface = std::destructible<T> && std::is_polymorphic_v<T>;

// Concept for factory interface
template<typename T, typename ServiceType>
concept FactoryInterface = requires(T factory) {
    { factory.create() } -> std::convertible_to<std::unique_ptr<ServiceType>>;
};

} // namespace liarsdice::interfaces
