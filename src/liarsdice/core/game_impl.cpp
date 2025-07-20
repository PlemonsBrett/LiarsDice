//
// Modern C++23 implementation of Game with Dependency Injection
// Created using industry best practices for testability and modularity
//

#include "liarsdice/core/game_impl.hpp"
#include "liarsdice/exceptions/game_exception.hpp"
#include <algorithm>
#include <format>
#include <ranges>
#include <sstream>

namespace liarsdice::core {

// Type aliases
using PlayerFactory = interfaces::ServiceFactory<interfaces::IPlayer>;

// Error message constants
namespace {
constexpr auto kInvalidGuessMsgGeneral = 
    "Invalid guess. You must either have more dice or a greater face value.";
constexpr auto kInvalidGuessMsgFaceValue = 
    "Invalid guess. You have the same number of dice but the face value is not greater.";
constexpr auto kInvalidGuessMsgDiceCount = 
    "Invalid guess. You have fewer dice but the face value is not greater than the last guess.";
constexpr auto kGameNotInitialized = "Game must be initialized before starting.";
constexpr auto kGameNotStarted = "Game has not been started yet.";
constexpr auto kPlayerNotFound = "Player not found with ID: {}";
constexpr auto kInvalidPlayerCount = "Player count must be between {} and {} players.";
} // namespace

GameImpl::GameImpl(
    std::unique_ptr<interfaces::IGameState> game_state,
    std::unique_ptr<interfaces::IRandomGenerator> random_generator,
    std::unique_ptr<PlayerFactory> player_factory
) : game_state_(std::move(game_state)),
    random_generator_(std::move(random_generator)),
    player_factory_(std::move(player_factory)) {
    
    if (!game_state_ || !random_generator_ || !player_factory_) {
        throw std::invalid_argument("All dependencies must be non-null");
    }
}

void GameImpl::initialize() {
    if (initialized_) {
        reset();
    }
    
    game_state_->set_game_active(false);
    game_state_->clear_last_guess();
    players_.clear();
    initialized_ = true;
}

void GameImpl::add_player(int player_id) {
    if (!initialized_) {
        throw std::runtime_error(kGameNotInitialized);
    }
    
    if (players_.size() >= kMaxPlayers) {
        throw std::runtime_error(std::format("Cannot add more than {} players", kMaxPlayers));
    }
    
    // Check if player already exists
    if (find_player_by_id(player_id) != players_.end()) {
        throw std::runtime_error(std::format("Player with ID {} already exists", player_id));
    }
    
    // Create new player using factory
    auto player = std::unique_ptr<interfaces::IPlayer>(
        static_cast<interfaces::IPlayer*>(player_factory_->create())
    );
    
    if (!player) {
        throw std::runtime_error("Failed to create player instance");
    }
    
    players_.push_back(std::move(player));
}

void GameImpl::start_game() {
    if (!initialized_) {
        throw std::runtime_error(kGameNotInitialized);
    }
    
    if (players_.size() < kMinPlayers) {
        throw std::runtime_error(std::format(kInvalidPlayerCount, kMinPlayers, kMaxPlayers));
    }
    
    initialize_players();
    game_state_->set_game_active(true);
    game_state_->clear_last_guess();
}

bool GameImpl::is_game_over() const {
    if (!game_state_->is_game_active()) {
        return false;
    }
    
    // Count active players
    auto active_count = std::ranges::count_if(players_, 
        [](const auto& player) { return player->is_active(); });
    
    return active_count <= 1;
}

int GameImpl::get_winner_id() const {
    if (!is_game_over()) {
        return -1;
    }
    
    return determine_winner();
}

std::string GameImpl::validate_guess(const interfaces::Guess& guess) const {
    if (!game_state_->is_game_active()) {
        return kGameNotStarted;
    }
    
    auto last_guess = game_state_->get_last_guess();
    return validate_guess_rules(guess, last_guess);
}

bool GameImpl::process_guess(const interfaces::Guess& guess) {
    auto validation_error = validate_guess(guess);
    if (!validation_error.empty()) {
        return false;
    }
    
    game_state_->set_last_guess(guess);
    game_state_->advance_to_next_player();
    return true;
}

std::string GameImpl::process_liar_call(int calling_player_id) {
    if (!game_state_->is_game_active()) {
        return "Game is not active";
    }
    
    auto last_guess_opt = game_state_->get_last_guess();
    if (!last_guess_opt.has_value()) {
        return "No guess has been made to call as a lie";
    }
    
    const auto& last_guess = last_guess_opt.value();
    
    // Count actual dice with the guessed face value
    auto actual_count = count_dice_with_value(last_guess.face_value);
    bool guess_was_correct = actual_count >= last_guess.dice_count;
    
    std::string result_message;
    
    if (guess_was_correct) {
        // Guess was correct, calling player loses a die
        auto calling_player_it = find_player_by_id(calling_player_id);
        if (calling_player_it != players_.end()) {
            (*calling_player_it)->remove_die();
            result_message = std::format(
                "Guess was correct! Player {} loses a die. Actual count: {}, Guessed: {}",
                calling_player_id, actual_count, last_guess.dice_count
            );
        } else {
            result_message = std::format(kPlayerNotFound, calling_player_id);
        }
    } else {
        // Guess was incorrect, guessing player loses a die
        auto guessing_player_it = find_player_by_id(last_guess.player_id);
        if (guessing_player_it != players_.end()) {
            (*guessing_player_it)->remove_die();
            result_message = std::format(
                "Guess was a lie! Player {} loses a die. Actual count: {}, Guessed: {}",
                last_guess.player_id, actual_count, last_guess.dice_count
            );
        } else {
            result_message = std::format(kPlayerNotFound, last_guess.player_id);
        }
    }
    
    // Remove eliminated players
    std::erase_if(players_, [](const auto& player) { 
        return !player->is_active(); 
    });
    
    // Clear the guess for next round
    game_state_->clear_last_guess();
    game_state_->increment_round();
    
    // Check if game is over
    if (is_game_over()) {
        int winner_id = get_winner_id();
        result_message += std::format(" Game Over! Winner: Player {}", winner_id);
        game_state_->set_game_active(false);
    }
    
    return result_message;
}

interfaces::IGameState& GameImpl::get_game_state() {
    return *game_state_;
}

const interfaces::IGameState& GameImpl::get_game_state() const {
    return *game_state_;
}

void GameImpl::reset() {
    players_.clear();
    game_state_->set_game_active(false);
    game_state_->clear_last_guess();
    initialized_ = false;
}

size_t GameImpl::get_min_players() const {
    return kMinPlayers;
}

size_t GameImpl::get_max_players() const {
    return kMaxPlayers;
}

size_t GameImpl::get_initial_dice_per_player() const {
    return kInitialDicePerPlayer;
}

// Private methods
void GameImpl::initialize_players() {
    for (auto& player : players_) {
        // Initialize each player with starting dice count
        for (size_t i = 0; i < kInitialDicePerPlayer; ++i) {
            player->add_die();
        }
        player->roll_dice();
    }
}

void GameImpl::eliminate_player(size_t player_index) {
    if (player_index < players_.size()) {
        players_.erase(players_.begin() + static_cast<std::ptrdiff_t>(player_index));
    }
}

int GameImpl::determine_winner() const {
    auto active_players = players_ | std::views::filter(
        [](const auto& player) { return player->is_active(); }
    );
    
    auto it = active_players.begin();
    return (it != active_players.end()) ? (*it)->get_id() : -1;
}

std::string GameImpl::validate_guess_rules(
    const interfaces::Guess& new_guess,
    const std::optional<interfaces::Guess>& last_guess_opt
) const {
    
    // Basic validation
    if (new_guess.face_value < 1 || new_guess.face_value > 6) {
        return "Face value must be between 1 and 6";
    }
    
    if (new_guess.dice_count == 0) {
        return "Dice count must be greater than 0";
    }
    
    auto total_dice = get_game_state().get_total_dice_count();
    if (new_guess.dice_count > total_dice) {
        return std::format("Cannot guess more dice ({}) than total remaining ({})", 
                          new_guess.dice_count, total_dice);
    }
    
    // If no previous guess, any valid guess is acceptable
    if (!last_guess_opt.has_value()) {
        return "";
    }
    
    const auto& last_guess = last_guess_opt.value();
    
    std::stringstream error_msg;
    error_msg << std::format("Last guess was ({}, {})\n", 
                             last_guess.dice_count, last_guess.face_value);
    
    // Rule validation against previous guess
    if (new_guess.dice_count < last_guess.dice_count && 
        new_guess.face_value <= last_guess.face_value) {
        error_msg << kInvalidGuessMsgDiceCount;
        return error_msg.str();
    }
    
    if (new_guess.dice_count == last_guess.dice_count && 
        new_guess.face_value <= last_guess.face_value) {
        error_msg << kInvalidGuessMsgFaceValue;
        return error_msg.str();
    }
    
    if (new_guess.dice_count <= last_guess.dice_count && 
        new_guess.face_value < last_guess.face_value) {
        error_msg << kInvalidGuessMsgGeneral;
        return error_msg.str();
    }
    
    return ""; // Valid guess
}

size_t GameImpl::count_dice_with_value(unsigned int face_value) const {
    size_t total_count = 0;
    
    for (const auto& player : players_) {
        if (player->is_active()) {
            total_count += player->count_dice_with_value(face_value);
        }
    }
    
    return total_count;
}

auto GameImpl::find_player_by_id(int player_id) -> decltype(players_.begin()) {
    return std::ranges::find_if(players_, 
        [player_id](const auto& player) { 
            return player->get_id() == player_id; 
        });
}

auto GameImpl::find_player_by_id(int player_id) const -> decltype(players_.cbegin()) {
    return std::ranges::find_if(players_, 
        [player_id](const auto& player) { 
            return player->get_id() == player_id; 
        });
}

} // namespace liarsdice::core