#pragma once

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace liarsdice::core {

class Dice {
public:
    static constexpr unsigned int MIN_VALUE = 1;
    static constexpr unsigned int MAX_VALUE = 6;
    
    Dice();
    explicit Dice(unsigned int initial_value);
    
    void roll();
    [[nodiscard]] unsigned int get_value() const noexcept { return value_; }
    
    // Set seed for deterministic testing
    static void set_seed(unsigned int seed);
    
    bool operator==(const Dice& other) const noexcept { return value_ == other.value_; }
    bool operator!=(const Dice& other) const noexcept { return !(*this == other); }
    
private:
    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & BOOST_SERIALIZATION_NVP(value_);
    }
    
    unsigned int value_;
    static thread_local boost::random::mt19937 rng_;
};

} // namespace liarsdice::core