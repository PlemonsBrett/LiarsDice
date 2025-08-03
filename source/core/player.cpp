#include <liarsdice/core/player.hpp>
#include <boost/log/trivial.hpp>
#include <algorithm>
#include <iostream>
#include <regex>

namespace liarsdice::core {

Player::Player(unsigned int id, const std::string& name) 
    : id_(id), name_(name.empty() ? "Player " + std::to_string(id) : name), points_(INITIAL_POINTS) {
    // Initialize with 5 dice
    dice_.reserve(INITIAL_DICE_COUNT);
    for (size_t i = 0; i < INITIAL_DICE_COUNT; ++i) {
        dice_.emplace_back();
    }
    BOOST_LOG_TRIVIAL(debug) << "Created player: " << name_ << " (ID: " << id_ << ") with " << points_ << " points";
}

void Player::roll_dice() {
    std::vector<unsigned int> values;
    values.reserve(dice_.size());
    
    for (auto& die : dice_) {
        die.roll();
        values.push_back(die.get_value());
    }
    
    BOOST_LOG_TRIVIAL(debug) << "Player " << name_ << " rolled dice";
    on_dice_rolled(values);
}

std::vector<unsigned int> Player::get_dice_values() const {
    std::vector<unsigned int> values;
    values.reserve(dice_.size());
    
    for (const auto& die : dice_) {
        values.push_back(die.get_value());
    }
    
    return values;
}

size_t Player::count_dice_with_value(unsigned int face_value) const {
    return std::count_if(dice_.begin(), dice_.end(),
        [face_value](const Dice& die) { return die.get_value() == face_value; });
}

void Player::add_die() {
    dice_.emplace_back();
    BOOST_LOG_TRIVIAL(debug) << "Player " << name_ << " gained a die (total: " << dice_.size() << ")";
}

bool Player::remove_die() {
    if (dice_.empty()) {
        return false;
    }
    
    dice_.pop_back();
    BOOST_LOG_TRIVIAL(debug) << "Player " << name_ << " lost a die (remaining: " << dice_.size() << ")";
    return true;
}

void Player::lose_points(int points_lost) {
    points_ -= points_lost;
    BOOST_LOG_TRIVIAL(info) << "Player " << name_ << " lost " << points_lost 
                           << " points (remaining: " << points_ << ")";
}

// HumanPlayer implementation
Guess HumanPlayer::make_guess(const std::optional<Guess>& last_guess) {
    std::cout << "\nYour dice: ";
    auto values = get_dice_values();
    std::sort(values.begin(), values.end());
    for (auto val : values) {
        std::cout << val << " ";
    }
    std::cout << "\n";
    
    if (last_guess) {
        std::cout << "Previous guess: " << last_guess->dice_count 
                  << " x " << last_guess->face_value << "\n";
    }
    
    unsigned int quantity, face_value;
    
    while (true) {
        std::cout << "Enter quantity: ";
        std::cin >> quantity;
        
        std::cout << "Enter face value (1-6): ";
        std::cin >> face_value;
        
        if (face_value < 1 || face_value > 6) {
            std::cout << "Invalid face value. Must be 1-6.\n";
            continue;
        }
        
        // Validate against previous guess
        if (last_guess) {
            bool valid = false;
            if (face_value > last_guess->face_value) {
                valid = quantity >= last_guess->dice_count;
            } else if (face_value == last_guess->face_value) {
                valid = quantity > last_guess->dice_count;
            }
            
            if (!valid) {
                std::cout << "Invalid guess. Must be higher than previous guess.\n";
                continue;
            }
        }
        
        break;
    }
    
    Guess guess{quantity, face_value, get_id()};
    on_guess(guess);
    return guess;
}

bool HumanPlayer::decide_call_liar(const Guess& last_guess) {
    std::cout << "\nYour dice: ";
    auto values = get_dice_values();
    std::sort(values.begin(), values.end());
    for (auto val : values) {
        std::cout << val << " ";
    }
    std::cout << "\n";
    
    std::cout << "Current guess: " << last_guess.dice_count 
              << " x " << last_guess.face_value << "\n";
    std::cout << "Call liar? (y/n): ";
    
    char response;
    std::cin >> response;
    
    bool calling_liar = (response == 'y' || response == 'Y');
    if (calling_liar) {
        on_call_liar();
    }
    
    return calling_liar;
}

} // namespace liarsdice::core