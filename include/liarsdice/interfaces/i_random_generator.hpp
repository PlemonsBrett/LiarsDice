#pragma once

namespace liarsdice::interfaces {

/**
 * @brief Interface for random number generation
 * 
 * Provides abstraction for random number generation to enable
 * deterministic testing and different RNG implementations.
 */
class IRandomGenerator {
public:
    virtual ~IRandomGenerator() = default;

    /**
     * @brief Generate a random integer within a range
     * @param min Minimum value (inclusive)
     * @param max Maximum value (inclusive)
     * @return Random integer between min and max
     */
    virtual int generate(int min, int max) = 0;

    /**
     * @brief Seed the random number generator
     * @param seed The seed value
     */
    virtual void seed(unsigned int seed) = 0;

    /**
     * @brief Generate a random boolean value
     * @return Random boolean
     */
    virtual bool generate_bool() = 0;

    /**
     * @brief Generate a random double between 0.0 and 1.0
     * @return Random double value
     */
    virtual double generate_normalized() = 0;
};

} // namespace liarsdice::interfaces