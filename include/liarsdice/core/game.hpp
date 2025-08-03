#pragma once

#include <liarsdice/core/player.hpp>
#include <boost/signals2.hpp>
#include <boost/log/trivial.hpp>
#include <memory>
#include <vector>
#include <optional>

namespace liarsdice::core {

// Game events for signal/slot system
struct GameEvents {
    boost::signals2::signal<void(unsigned int round)> on_round_start;
    boost::signals2::signal<void(unsigned int round)> on_round_end;
    boost::signals2::signal<void(const Player&)> on_player_turn;
    boost::signals2::signal<void(const Player&, const Guess&)> on_guess_made;
    boost::signals2::signal<void(const Player&)> on_liar_called;
    boost::signals2::signal<void(const Player&, const Player&)> on_round_result;
    boost::signals2::signal<void(const Player&)> on_player_eliminated;
    boost::signals2::signal<void(const Player&)> on_game_winner;
};

// Game configuration
struct GameConfig {
    unsigned int min_players = 2;
    unsigned int max_players = 8;
    unsigned int starting_dice = 5;
    bool allow_ones_wild = true;
    unsigned int ai_think_time_ms = 1000;
};

class Game {
public:
    enum class State {
        NOT_STARTED,
        WAITING_FOR_PLAYERS,
        IN_PROGRESS,
        ROUND_ENDED,
        GAME_OVER
    };
    
    explicit Game(const GameConfig& config = {});
    
    // Player management
    void add_player(std::shared_ptr<Player> player);
    void remove_player(unsigned int player_id);
    [[nodiscard]] size_t get_player_count() const { return players_.size(); }
    [[nodiscard]] const std::vector<std::shared_ptr<Player>>& get_players() const { return players_; }
    [[nodiscard]] std::shared_ptr<Player> get_player(unsigned int id) const;
    [[nodiscard]] std::shared_ptr<Player> get_current_player() const;
    
    // Game flow
    void start_game();
    void start_round();
    void process_guess(const Guess& guess);
    void process_call_liar();
    void end_round();
    void end_game();
    
    // Game state
    [[nodiscard]] State get_state() const { return state_; }
    [[nodiscard]] bool is_game_active() const { return state_ == State::IN_PROGRESS; }
    [[nodiscard]] unsigned int get_round_number() const { return round_number_; }
    [[nodiscard]] std::optional<Guess> get_last_guess() const { return last_guess_; }
    [[nodiscard]] size_t get_total_dice_count() const;
    [[nodiscard]] size_t count_total_dice_with_value(unsigned int face_value) const;
    
    // Event access
    GameEvents& events() { return events_; }
    const GameEvents& events() const { return events_; }
    
private:
    GameConfig config_;
    State state_ = State::NOT_STARTED;
    std::vector<std::shared_ptr<Player>> players_;
    std::vector<std::shared_ptr<Player>> active_players_;
    size_t current_player_index_ = 0;
    unsigned int round_number_ = 0;
    std::optional<Guess> last_guess_;
    GameEvents events_;
    
    void advance_to_next_player();
    void eliminate_player(std::shared_ptr<Player> player);
    bool is_valid_guess(const Guess& guess) const;
    void log_game_state() const;
};

// Stream operator for Game::State (needed for Boost.Test)
inline std::ostream& operator<<(std::ostream& os, Game::State state) {
    switch (state) {
        case Game::State::NOT_STARTED: return os << "NOT_STARTED";
        case Game::State::WAITING_FOR_PLAYERS: return os << "WAITING_FOR_PLAYERS";
        case Game::State::IN_PROGRESS: return os << "IN_PROGRESS";
        case Game::State::ROUND_ENDED: return os << "ROUND_ENDED";
        case Game::State::GAME_OVER: return os << "GAME_OVER";
        default: return os << "UNKNOWN";
    }
}

} // namespace liarsdice::core