#pragma once

#include <liarsdice/core/dice.hpp>
#include <boost/signals2.hpp>
#include <boost/serialization/access.hpp>
#include <memory>
#include <string>
#include <vector>

namespace liarsdice::core {

struct Guess {
    unsigned int dice_count;
    unsigned int face_value;
    unsigned int player_id;
    
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & dice_count & face_value & player_id;
    }
};

class Player {
public:
    using GuessSignal = boost::signals2::signal<void(const Guess&)>;
    using CallLiarSignal = boost::signals2::signal<void()>;
    using DiceRolledSignal = boost::signals2::signal<void(const std::vector<unsigned int>&)>;
    
    explicit Player(unsigned int id, const std::string& name = "");
    virtual ~Player() = default;
    
    // Core functionality
    void roll_dice();
    [[nodiscard]] size_t get_dice_count() const { return dice_.size(); }
    [[nodiscard]] unsigned int get_id() const { return id_; }
    [[nodiscard]] const std::string& get_name() const { return name_; }
    [[nodiscard]] bool has_dice() const { return !dice_.empty(); }
    [[nodiscard]] std::vector<unsigned int> get_dice_values() const;
    [[nodiscard]] size_t count_dice_with_value(unsigned int face_value) const;
    
    // Game actions
    virtual Guess make_guess(const std::optional<Guess>& last_guess) = 0;
    virtual bool decide_call_liar(const Guess& last_guess) = 0;
    
    // Dice management
    void add_die();
    bool remove_die();
    
    // Signals
    GuessSignal on_guess;
    CallLiarSignal on_call_liar;
    DiceRolledSignal on_dice_rolled;
    
protected:
    [[nodiscard]] const std::vector<Dice>& get_dice() const { return dice_; }
    
private:
    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & id_ & name_ & dice_;
    }
    
    unsigned int id_;
    std::string name_;
    std::vector<Dice> dice_;
    static constexpr size_t INITIAL_DICE_COUNT = 5;
};

// Human player implementation
class HumanPlayer : public Player {
public:
    using Player::Player;
    
    Guess make_guess(const std::optional<Guess>& last_guess) override;
    bool decide_call_liar(const Guess& last_guess) override;
};

} // namespace liarsdice::core