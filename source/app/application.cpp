#include <liarsdice/app/application.hpp>
#include <boost/log/expressions.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <sstream>
#include <limits>
#include <iomanip>
#include <thread>
#include <chrono>

namespace liarsdice::app {

// Static member definitions
std::atomic<bool> Application::should_exit{false};

Application::Application() {
    setup_signal_handlers();
}

int Application::run(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        if (!parse_command_line(argc, argv, config_)) {
            return 1;
        }
        
        init_logging();
        
        BOOST_LOG_TRIVIAL(info) << "=== Liar's Dice v1.0 (Boost Edition) ===";
        BOOST_LOG_TRIVIAL(info) << "Application started";
        
        // Main application loop
        show_welcome();
        
        bool play_again = true;
        while (play_again && !should_exit.load()) {
            try {
                run_interactive_session();
                
                // Exit immediately if quit was requested
                if (should_exit.load()) {
                    break;
                }
                
                // Ask if they want to play again
                std::cout << "Do you want to play again? (yes/no): ";
                std::string response = get_line_input();
                play_again = (response == "yes" || response == "y");
                
            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << "Game error: " << e.what();
                std::cout << "An error occurred. Please try again.\n";
            }
        }
        
        handle_quit();
        return 0;
        
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(fatal) << "Fatal error: " << e.what();
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}

void Application::signal_handler(int signal) {
    BOOST_LOG_TRIVIAL(info) << "Received signal: " << signal;
    should_exit.store(true);
    
    switch (signal) {
        case SIGINT:
            std::cout << "\nInterrupted. Exiting gracefully...\n";
            break;
        case SIGTERM:
            std::cout << "\nTerminated. Exiting gracefully...\n";
            break;
        default:
            std::cout << "\nReceived signal " << signal << ". Exiting...\n";
            break;
    }
    
    std::cout << GOODBYE_MSG << std::endl;
    std::exit(0);
}

void Application::init_logging() {
    boost::log::core::get()->set_filter(
        boost::log::trivial::severity >= (config_.verbose ? 
            boost::log::trivial::debug : boost::log::trivial::info)
    );
    
    BOOST_LOG_TRIVIAL(info) << "Logging initialized. Verbose: " << config_.verbose;
}

void Application::setup_signal_handlers() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    BOOST_LOG_TRIVIAL(debug) << "Signal handlers installed";
}

po::options_description Application::setup_program_options() {
    po::options_description desc("Liar's Dice CLI Options");
    desc.add_options()
        ("help,h", "Show help message")
        ("verbose,v", "Enable verbose output")
        ("demo", "Run in demo mode")
        ("min-players", po::value<unsigned int>(&config_.min_players)->default_value(2), 
         "Minimum number of players")
        ("max-players", po::value<unsigned int>(&config_.max_players)->default_value(8), 
         "Maximum number of players")
        ("starting-dice", po::value<unsigned int>(&config_.starting_dice)->default_value(5), 
         "Number of dice each player starts with")
        ("ai-think-time", po::value<unsigned int>(&config_.ai_think_time)->default_value(1000), 
         "AI thinking time in milliseconds")
        ("seed", po::value<unsigned int>(), 
         "Random seed for deterministic gameplay (useful for testing)");
    
    return desc;
}

bool Application::parse_command_line(int argc, char* argv[], AppConfig& config) {
    try {
        auto desc = setup_program_options();
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return false;
        }
        
        config.verbose = vm.count("verbose");
        config.demo_mode = vm.count("demo");
        
        // Handle optional seed
        if (vm.count("seed")) {
            config.random_seed = vm["seed"].as<unsigned int>();
        }
        
        return true;
        
    } catch (const po::error& e) {
        std::cerr << "Command line error: " << e.what() << std::endl;
        return false;
    }
}

void Application::show_welcome() {
    print_with_flush(WELCOME_MSG);
}

void Application::run_interactive_session() {
    // Get player counts
    int total_players = get_player_count();
    if (should_exit.load()) return;
    
    int ai_players = get_ai_player_count(total_players);
    if (should_exit.load()) return;
    
    // Setup and start game
    setup_game(total_players, ai_players);
    if (should_exit.load()) return;
    
    play_game();
}

int Application::get_player_count() {
    return get_validated_input(PLAYER_COUNT_PROMPT, config_.min_players, config_.max_players);
}

int Application::get_ai_player_count(int total_players) {
    return get_validated_input(AI_COUNT_PROMPT, 0, total_players - 1);
}

void Application::setup_game(int total_players, int ai_players) {
    BOOST_LOG_TRIVIAL(info) << "Setting up game: " << total_players 
                           << " total players, " << ai_players << " AI players";
    
    // Create game with configuration
    core::GameConfig game_config;
    game_config.min_players = config_.min_players;
    game_config.max_players = config_.max_players;
    game_config.starting_dice = config_.starting_dice;
    game_config.allow_ones_wild = config_.allow_ones_wild;
    game_config.ai_think_time_ms = config_.ai_think_time;
    game_config.random_seed = config_.random_seed;  // Pass seed if provided
    
    game_ = std::make_unique<core::Game>(game_config);
    players_.clear();
    next_player_id_ = 1;
    
    // Create human players
    int human_players = total_players - ai_players;
    for (int i = 0; i < human_players; ++i) {
        auto human = std::make_shared<core::HumanPlayer>(next_player_id_++, 
                                                        "Player " + std::to_string(i + 1));
        players_.push_back(human);
        game_->add_player(human);
    }
    
    // Create AI players
    for (int i = 0; i < ai_players; ++i) {
        auto ai = std::make_shared<ai::EasyAI>(next_player_id_++);
        players_.push_back(ai);
        game_->add_player(ai);
    }
    
    // Connect to game events
    game_->events().on_round_start.connect([this](unsigned int round) {
        on_round_start(round);
    });
    
    game_->events().on_player_turn.connect([this](const core::Player& player) {
        on_player_turn(player);
    });
    
    game_->events().on_guess_made.connect([this](const core::Player& player, const core::Guess& guess) {
        on_guess_made(player, guess);
    });
    
    game_->events().on_liar_called.connect([this](const core::Player& player) {
        on_liar_called(player);
    });
    
    game_->events().on_round_result.connect([this](const core::Player& caller, const core::Player& loser, int points_lost) {
        on_round_result(caller, loser, points_lost);
    });
    
    game_->events().on_player_eliminated.connect([this](const core::Player& player) {
        on_player_eliminated(player);
    });
    
    game_->events().on_game_winner.connect([this](const core::Player& winner) {
        on_game_winner(winner);
    });
    
    print_with_flush(GAME_STARTING_MSG);
}

void Application::play_game() {
    game_running_ = true;
    game_->start_game();
    
    // Game loop - the events handle the flow
    while (game_running_ && !should_exit.load() && game_->is_game_active()) {
        auto current_player = game_->get_current_player();
        if (!current_player) break;
        
        // Check if it's a human player's turn
        if (auto human = std::dynamic_pointer_cast<core::HumanPlayer>(current_player)) {
            handle_human_turn(*human);
        } else {
            // AI player - let the game handle it automatically
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Brief pause for readability
            
            // AI makes decision
            auto last_guess = game_->get_last_guess();
            if (last_guess.has_value()) {
                // AI decides whether to call liar or make a guess
                bool calls_liar = current_player->decide_call_liar(*last_guess);
                if (calls_liar) {
                    game_->process_call_liar();
                } else {
                    auto guess = current_player->make_guess(last_guess);
                    game_->process_guess(guess);
                }
            } else {
                // First turn - make a guess
                auto guess = current_player->make_guess(std::nullopt);
                game_->process_guess(guess);
            }
        }
    }
    
    game_running_ = false;
}

int Application::get_validated_input(const std::string& prompt, int min, int max) {
    while (!should_exit.load()) {
        std::cout << prompt << " (" << min << "-" << max << "): ";
        std::cout.flush();
        
        std::string input = get_line_input();
        if (should_exit.load() || is_quit_command(input)) {
            should_exit.store(true);
            return min; // Return valid value to avoid issues
        }
        
        try {
            int value = std::stoi(input);
            if (value >= min && value <= max) {
                return value;
            } else {
                print_with_flush(INVALID_MSG);
            }
        } catch (const std::exception&) {
            print_with_flush(INVALID_MSG);
        }
    }
    return min;
}

std::string Application::get_line_input() {
    std::string line;
    if (!std::getline(std::cin, line)) {
        // EOF or error
        should_exit.store(true);
        return "";
    }
    
    // Trim whitespace
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    line.erase(line.find_last_not_of(" \t\r\n") + 1);
    
    return line;
}

bool Application::is_quit_command(const std::string& input) {
    return input == "q" || input == "quit" || input == "exit";
}

void Application::handle_quit() {
    print_with_flush(GOODBYE_MSG);
    BOOST_LOG_TRIVIAL(info) << "Application terminated normally";
}

void Application::handle_human_turn(core::Player& player) {
    show_player_dice(player);
    
    std::cout << YOUR_TURN_MSG << " - " << player.get_name() << std::endl;
    std::cout << "1. Make a guess" << std::endl;
    
    auto last_guess = game_->get_last_guess();
    if (last_guess.has_value()) {
        std::cout << "2. Call liar" << std::endl;
        std::cout << "Previous guess: " << last_guess->dice_count 
                  << " dice showing " << last_guess->face_value << std::endl;
    }
    
    int choice = get_player_choice();
    
    if (choice == 1) {
        // Make a guess
        auto guess = get_player_guess(last_guess);
        game_->process_guess(guess);
    } else if (choice == 2 && last_guess.has_value()) {
        // Call liar
        game_->process_call_liar();
    }
}

int Application::get_player_choice() {
    auto last_guess = game_->get_last_guess();
    int max_choice = last_guess.has_value() ? 2 : 1;
    
    return get_validated_input("Choose", 1, max_choice);
}

core::Guess Application::get_player_guess(const std::optional<core::Guess>& last_guess) {
    int dice_count, face_value;
    
    while (!should_exit.load()) {
        dice_count = get_validated_input(DICE_COUNT_PROMPT, 1, 50); // Reasonable upper limit
        if (should_exit.load()) break;
        
        face_value = get_validated_input(FACE_VALUE_PROMPT, 1, 6);
        if (should_exit.load()) break;
        
        auto current_player = game_->get_current_player();
        core::Guess guess{static_cast<unsigned int>(dice_count), 
                         static_cast<unsigned int>(face_value), 
                         current_player ? current_player->get_id() : 0};
        
        // Use game's validation
        if (!game_->is_valid_guess(guess)) {
            print_with_flush(INVALID_MSG);
            if (last_guess.has_value()) {
                std::cout << "You must either increase the dice count or keep the same count with a higher face value." << std::endl;
                std::cout << "Previous guess was: " << last_guess->dice_count 
                         << " dice showing " << last_guess->face_value << std::endl;
            }
            continue;
        }
        
        return guess;
    }
    
    // Return a dummy guess if exiting
    return core::Guess{1, 1, 0};
}

void Application::show_game_state() {
    std::cout << "\n=== Game State ===" << std::endl;
    std::cout << "Round: " << game_->get_round_number() << std::endl;
    std::cout << "Total dice in play: " << game_->get_total_dice_count() << std::endl;
    
    for (const auto& player : game_->get_players()) {
        if (!player->is_eliminated()) {
            std::cout << player->get_name() << ": " << player->get_points() << " points" << std::endl;
        }
    }
    
    auto last_guess = game_->get_last_guess();
    if (last_guess.has_value()) {
        std::cout << "Last guess: " << last_guess->dice_count 
                  << " dice showing " << last_guess->face_value << std::endl;
    }
    std::cout << "==================\n" << std::endl;
}

void Application::show_player_dice(const core::Player& player) {
    auto dice_values = player.get_dice_values();
    std::cout << "\nYour dice: ";
    for (size_t i = 0; i < dice_values.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << dice_values[i];
    }
    std::cout << std::endl;
}

void Application::clear_screen() {
    // Simple cross-platform screen clear
    std::cout << "\033[2J\033[1;1H";
}

void Application::print_with_flush(const std::string& message) {
    std::cout << message << std::endl;
    std::cout.flush();
}

// Event handlers
void Application::on_round_start(unsigned int round) {
    std::cout << "\n--- Round " << round << " ---" << std::endl;
    show_game_state();
}

void Application::on_player_turn(const core::Player& player) {
    if (auto human = dynamic_cast<const core::HumanPlayer*>(&player)) {
        // Human turn handled in main loop
    } else {
        std::cout << player.get_name() << "'s turn..." << std::endl;
    }
}

void Application::on_guess_made(const core::Player& player, const core::Guess& guess) {
    std::cout << player.get_name() << " guesses: " 
              << guess.dice_count << " dice showing " << guess.face_value << std::endl;
}

void Application::on_liar_called(const core::Player& player) {
    std::cout << player.get_name() << " calls LIAR!" << std::endl;
}

void Application::on_round_result(const core::Player& caller, const core::Player& loser, int points_lost) {
    std::cout << loser.get_name() << " loses " << points_lost << " point" 
              << (points_lost > 1 ? "s" : "") << "!" << std::endl;
    std::cout << loser.get_name() << " now has " << loser.get_points() << " points remaining." << std::endl;
}

void Application::on_player_eliminated(const core::Player& player) {
    std::cout << player.get_name() << " has been eliminated!" << std::endl;
}

void Application::on_game_winner(const core::Player& winner) {
    std::cout << "\n" << winner.get_name() << " " << WINS_MSG << "!" << std::endl;
    game_running_ = false;
}

} // namespace liarsdice::app