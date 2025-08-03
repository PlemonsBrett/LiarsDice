#include <liarsdice/core/game.hpp>
#include <liarsdice/core/dice.hpp>
#include <boost/log/trivial.hpp>
#include <algorithm>
#include <random>
#include <thread>
#include <chrono>

namespace liarsdice::core {

Game::Game(const GameConfig& config) : config_(config) {
    BOOST_LOG_TRIVIAL(info) << "Game created with config: min_players=" << config_.min_players 
                           << ", max_players=" << config_.max_players 
                           << ", starting_dice=" << config_.starting_dice;
    
    // Set random seed if provided
    if (config_.random_seed.has_value()) {
        Dice::set_seed(config_.random_seed.value());
        BOOST_LOG_TRIVIAL(info) << "Random seed set to: " << config_.random_seed.value();
    }
}

void Game::add_player(std::shared_ptr<Player> player) {
    if (!player) {
        BOOST_LOG_TRIVIAL(warning) << "Attempted to add null player";
        return;
    }
    
    if (state_ != State::NOT_STARTED && state_ != State::WAITING_FOR_PLAYERS) {
        BOOST_LOG_TRIVIAL(warning) << "Cannot add player " << player->get_id() 
                                  << " - game already in progress";
        return;
    }
    
    if (players_.size() >= config_.max_players) {
        BOOST_LOG_TRIVIAL(warning) << "Cannot add player " << player->get_id() 
                                  << " - maximum players reached";
        return;
    }
    
    // Check for duplicate player ID
    auto existing = std::find_if(players_.begin(), players_.end(),
        [&](const auto& p) { return p->get_id() == player->get_id(); });
    
    if (existing != players_.end()) {
        BOOST_LOG_TRIVIAL(warning) << "Player with ID " << player->get_id() 
                                  << " already exists";
        return;
    }
    
    players_.push_back(player);
    
    // Adjust dice count to match game configuration
    while (player->get_dice_count() > config_.starting_dice) {
        player->remove_die();
    }
    while (player->get_dice_count() < config_.starting_dice) {
        player->add_die();
    }
    
    BOOST_LOG_TRIVIAL(info) << "Added player " << player->get_id() 
                           << " (" << player->get_name() << ") with " 
                           << player->get_dice_count() << " dice";
    
    if (state_ == State::NOT_STARTED) {
        state_ = State::WAITING_FOR_PLAYERS;
    }
    
    if (players_.size() >= config_.min_players) {
        BOOST_LOG_TRIVIAL(info) << "Game ready to start - minimum players reached";
    }
}

void Game::remove_player(unsigned int player_id) {
    auto it = std::find_if(players_.begin(), players_.end(),
        [&](const auto& p) { return p->get_id() == player_id; });
    
    if (it == players_.end()) {
        BOOST_LOG_TRIVIAL(warning) << "Player " << player_id << " not found for removal";
        return;
    }
    
    BOOST_LOG_TRIVIAL(info) << "Removing player " << player_id;
    players_.erase(it);
    
    // Remove from active players if present
    auto active_it = std::find_if(active_players_.begin(), active_players_.end(),
        [&](const auto& p) { return p->get_id() == player_id; });
    if (active_it != active_players_.end()) {
        active_players_.erase(active_it);
    }
    
    // Check if game should end
    if (state_ == State::IN_PROGRESS && active_players_.size() <= 1) {
        end_game();
    }
}

std::shared_ptr<Player> Game::get_player(unsigned int id) const {
    auto it = std::find_if(players_.begin(), players_.end(),
        [&](const auto& p) { return p->get_id() == id; });
    return (it != players_.end()) ? *it : nullptr;
}

std::shared_ptr<Player> Game::get_current_player() const {
    if (active_players_.empty() || current_player_index_ >= active_players_.size()) {
        return nullptr;
    }
    return active_players_[current_player_index_];
}

void Game::start_game() {
    if (state_ != State::WAITING_FOR_PLAYERS && state_ != State::NOT_STARTED) {
        BOOST_LOG_TRIVIAL(warning) << "Cannot start game - invalid state";
        return;
    }
    
    if (players_.size() < config_.min_players) {
        BOOST_LOG_TRIVIAL(warning) << "Cannot start game - not enough players ("
                                  << players_.size() << "/" << config_.min_players << ")";
        return;
    }
    
    BOOST_LOG_TRIVIAL(info) << "Starting game with " << players_.size() << " players";
    
    // Initialize active players
    active_players_ = players_;
    
    // Shuffle player order
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(active_players_.begin(), active_players_.end(), gen);
    
    state_ = State::IN_PROGRESS;
    round_number_ = 0;
    current_player_index_ = 0;
    
    start_round();
}

void Game::start_round() {
    if (state_ != State::IN_PROGRESS) {
        BOOST_LOG_TRIVIAL(warning) << "Cannot start round - game not in progress";
        return;
    }
    
    if (active_players_.size() <= 1) {
        end_game();
        return;
    }
    
    ++round_number_;
    last_guess_.reset();
    
    BOOST_LOG_TRIVIAL(info) << "Starting round " << round_number_;
    
    // All players roll their dice
    for (auto& player : active_players_) {
        player->roll_dice();
    }
    
    events_.on_round_start(round_number_);
    log_game_state();
    
    // Signal first player's turn
    if (!active_players_.empty()) {
        events_.on_player_turn(*get_current_player());
    }
}

void Game::process_guess(const Guess& guess) {
    if (state_ != State::IN_PROGRESS) {
        BOOST_LOG_TRIVIAL(warning) << "Cannot process guess - game not in progress";
        return;
    }
    
    auto current_player = get_current_player();
    if (!current_player || current_player->get_id() != guess.player_id) {
        BOOST_LOG_TRIVIAL(warning) << "Invalid guess - not current player's turn";
        return;
    }
    
    if (!is_valid_guess(guess)) {
        BOOST_LOG_TRIVIAL(warning) << "Invalid guess received";
        return;
    }
    
    BOOST_LOG_TRIVIAL(info) << "Player " << guess.player_id << " guesses: "
                           << guess.dice_count << " dice showing " << guess.face_value;
    
    last_guess_ = guess;
    events_.on_guess_made(*current_player, guess);
    
    advance_to_next_player();
    
    // Signal next player's turn
    if (auto next_player = get_current_player()) {
        events_.on_player_turn(*next_player);
    }
}

void Game::process_call_liar() {
    if (state_ != State::IN_PROGRESS) {
        BOOST_LOG_TRIVIAL(warning) << "Cannot call liar - game not in progress";
        return;
    }
    
    if (!last_guess_.has_value()) {
        BOOST_LOG_TRIVIAL(warning) << "Cannot call liar - no previous guess";
        return;
    }
    
    auto current_player = get_current_player();
    if (!current_player) {
        BOOST_LOG_TRIVIAL(warning) << "Cannot call liar - no current player";
        return;
    }
    
    BOOST_LOG_TRIVIAL(info) << "Player " << current_player->get_id() << " calls liar!";
    events_.on_liar_called(*current_player);
    
    // Count actual dice matching the guess
    auto actual_count = count_total_dice_with_value(last_guess_->face_value);
    
    // Include ones as wild if enabled (unless the guess was for ones)
    if (config_.allow_ones_wild && last_guess_->face_value != 1) {
        actual_count += count_total_dice_with_value(1);
    }
    
    BOOST_LOG_TRIVIAL(info) << "Actual count: " << actual_count 
                           << ", Guessed: " << last_guess_->dice_count;
    
    // Determine who loses points
    std::shared_ptr<Player> loser;
    int points_lost = 0;
    
    if (actual_count >= last_guess_->dice_count) {
        // Guess was correct, caller loses (false accusation)
        loser = current_player;
        points_lost = 2; // Lose 2 points for false accusation
        BOOST_LOG_TRIVIAL(info) << "Liar call failed - caller loses " << points_lost << " points for false accusation";
    } else {
        // Guess was wrong, guesser loses (caught lying)
        loser = get_player(last_guess_->player_id);
        points_lost = 1; // Lose 1 point for being caught lying
        BOOST_LOG_TRIVIAL(info) << "Liar call succeeded - guesser loses " << points_lost << " point for lying";
    }
    
    if (loser) {
        loser->lose_points(points_lost);
        events_.on_round_result(*current_player, *loser, points_lost);
        
        // Check if player is eliminated
        if (loser->is_eliminated()) {
            eliminate_player(loser);
        }
    }
    
    end_round();
}

void Game::end_round() {
    BOOST_LOG_TRIVIAL(info) << "Round " << round_number_ << " ended";
    events_.on_round_end(round_number_);
    
    state_ = State::ROUND_ENDED;
    
    // Check for game end condition
    if (active_players_.size() <= 1) {
        end_game();
    } else {
        // Continue to next round
        state_ = State::IN_PROGRESS;
        start_round();
    }
}

void Game::end_game() {
    state_ = State::GAME_OVER;
    
    if (!active_players_.empty()) {
        auto winner = active_players_[0];
        BOOST_LOG_TRIVIAL(info) << "Game over! Winner: " << winner->get_name() 
                               << " (Player " << winner->get_id() << ")";
        events_.on_game_winner(*winner);
    } else {
        BOOST_LOG_TRIVIAL(info) << "Game over - no winner";
    }
}

size_t Game::get_total_dice_count() const {
    size_t total = 0;
    for (const auto& player : active_players_) {
        total += player->get_dice_count();
    }
    return total;
}

size_t Game::count_total_dice_with_value(unsigned int face_value) const {
    size_t count = 0;
    for (const auto& player : active_players_) {
        count += player->count_dice_with_value(face_value);
    }
    return count;
}

void Game::advance_to_next_player() {
    if (active_players_.empty()) return;
    
    current_player_index_ = (current_player_index_ + 1) % active_players_.size();
}

void Game::eliminate_player(std::shared_ptr<Player> player) {
    BOOST_LOG_TRIVIAL(info) << "Player " << player->get_id() 
                           << " (" << player->get_name() << ") eliminated";
    
    events_.on_player_eliminated(*player);
    
    // Remove from active players
    auto it = std::find(active_players_.begin(), active_players_.end(), player);
    if (it != active_players_.end()) {
        size_t removed_index = std::distance(active_players_.begin(), it);
        active_players_.erase(it);
        
        // Adjust current player index if necessary
        if (current_player_index_ >= active_players_.size() && !active_players_.empty()) {
            current_player_index_ = 0;
        } else if (removed_index < current_player_index_) {
            --current_player_index_;
        }
    }
}

bool Game::is_valid_guess(const Guess& guess) const {
    // Basic validation
    if (guess.face_value < 1 || guess.face_value > 6) {
        return false;
    }
    
    if (guess.dice_count == 0) {
        return false;
    }
    
    // Must be higher than previous guess
    if (last_guess_.has_value()) {
        const auto& prev = *last_guess_;
        
        // Valid moves in Liar's Dice:
        // 1. Increase quantity (dice count) - face value can be anything
        if (guess.dice_count > prev.dice_count) {
            return true;
        }
        
        // 2. Same quantity, but must increase face value
        if (guess.dice_count == prev.dice_count && guess.face_value > prev.face_value) {
            return true;
        }
        
        // All other cases are invalid (can't decrease quantity)
        return false;
    }
    
    return true;
}

void Game::log_game_state() const {
    BOOST_LOG_TRIVIAL(debug) << "=== Game State ===";
    BOOST_LOG_TRIVIAL(debug) << "Round: " << round_number_;
    BOOST_LOG_TRIVIAL(debug) << "Active players: " << active_players_.size();
    BOOST_LOG_TRIVIAL(debug) << "Total dice: " << get_total_dice_count();
    
    for (const auto& player : active_players_) {
        BOOST_LOG_TRIVIAL(debug) << "  Player " << player->get_id() 
                                << " (" << player->get_name() << "): "
                                << player->get_dice_count() << " dice";
    }
    
    if (last_guess_.has_value()) {
        BOOST_LOG_TRIVIAL(debug) << "Last guess: " << last_guess_->dice_count 
                                << " dice showing " << last_guess_->face_value;
    }
    BOOST_LOG_TRIVIAL(debug) << "==================";
}

} // namespace liarsdice::core