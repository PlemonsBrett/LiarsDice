#include <liarsdice/core/dice.hpp>
#include <chrono>

namespace liarsdice::core {

thread_local boost::random::mt19937 Dice::rng_{
    static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count())
};

Dice::Dice() : value_(MIN_VALUE) {
    roll();
}

Dice::Dice(unsigned int initial_value) : value_(initial_value) {
    if (value_ < MIN_VALUE || value_ > MAX_VALUE) {
        throw std::invalid_argument("Dice value must be between 1 and 6");
    }
}

void Dice::roll() {
    boost::random::uniform_int_distribution<unsigned int> dist(MIN_VALUE, MAX_VALUE);
    value_ = dist(rng_);
}

} // namespace liarsdice::core