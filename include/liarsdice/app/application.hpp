#pragma once

#include <liarsdice/core/game.hpp>
#include <liarsdice/core/player.hpp>
#include <liarsdice/ai/ai_player.hpp>
#include <liarsdice/ui/ui_config.hpp>
#include <liarsdice/di/service_container.hpp>
#include <boost/program_options.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <csignal>

namespace liarsdice::app {

namespace po = boost::program_options;

struct AppConfig {
    std::string ui_config_path = "assets/ui/default.info";
    bool verbose = false;
    bool demo_mode = false;
    bool use_medium_ai = false;
    unsigned int min_players = 2;
    unsigned int max_players = 8;
    unsigned int starting_dice = 5;
    bool allow_ones_wild = true;
    unsigned int ai_think_time = 1000;
    std::optional<unsigned int> random_seed;  // For deterministic testing
};

class Application {
public:
    Application();
    ~Application() = default;
    
    // Main application entry point
    int run(int argc, char* argv[]);
    
    // Signal handling
    static void signal_handler(int signal);
    static std::atomic<bool> should_exit;
    
private:
    // Initialization
    void init_logging();
    void setup_signal_handlers();
    po::options_description setup_program_options();
    bool parse_command_line(int argc, char* argv[], AppConfig& config);
    
    // CLI Interface (Robot Framework compatible)
    void show_welcome();
    void run_interactive_session();
    int get_player_count();
    int get_ai_player_count(int total_players);
    void setup_game(int total_players, int ai_players);
    void play_game();
    
    // Input handling
    int get_validated_input(const std::string& prompt, int min, int max);
    std::string get_line_input();
    bool is_quit_command(const std::string& input);
    void handle_quit();
    
    // Player turn interface
    void handle_human_turn(core::Player& player);
    int get_player_choice();
    core::Guess get_player_guess(const std::optional<core::Guess>& last_guess);
    
    // Game flow
    void show_game_state();
    void show_player_dice(const core::Player& player);
    void clear_screen();
    
    // Event handlers for game feedback
    void on_round_start(unsigned int round);
    void on_player_turn(const core::Player& player);
    void on_guess_made(const core::Player& player, const core::Guess& guess);
    void on_liar_called(const core::Player& player);
    void on_round_result(const core::Player& caller, const core::Player& loser, int points_lost);
    void on_player_eliminated(const core::Player& player);
    void on_game_winner(const core::Player& winner);
    
    // Utility
    void print_with_flush(const std::string& message);
    
private:
    AppConfig config_;
    std::unique_ptr<ui::UIConfig> ui_config_;
    std::unique_ptr<core::Game> game_;
    std::vector<std::shared_ptr<core::Player>> players_;
    unsigned int next_player_id_ = 1;
    bool game_running_ = false;
    
    // Constants for Robot Framework compatibility
    static constexpr const char* WELCOME_MSG = "Welcome to Liar's Dice";
    static constexpr const char* PLAYER_COUNT_PROMPT = "Enter the number of players";
    static constexpr const char* AI_COUNT_PROMPT = "How many AI players";
    static constexpr const char* INVALID_MSG = "Invalid";
    static constexpr const char* YOUR_TURN_MSG = "Your turn";
    static constexpr const char* GAME_STARTING_MSG = "Game starting";
    static constexpr const char* DICE_COUNT_PROMPT = "dice count";
    static constexpr const char* FACE_VALUE_PROMPT = "face value";
    static constexpr const char* WINS_MSG = "wins the game";
    static constexpr const char* GOODBYE_MSG = "Goodbye";
};

} // namespace liarsdice::app