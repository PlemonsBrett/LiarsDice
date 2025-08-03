#define BOOST_TEST_MODULE DiceTests
#include <boost/test/unit_test.hpp>
#include <liarsdice/core/dice.hpp>
#include <set>

using namespace liarsdice::core;

BOOST_AUTO_TEST_SUITE(DiceTestSuite)

BOOST_AUTO_TEST_CASE(DiceConstruction) {
    Dice d1;
    BOOST_CHECK(d1.get_value() >= Dice::MIN_VALUE);
    BOOST_CHECK(d1.get_value() <= Dice::MAX_VALUE);
    
    Dice d2(3);
    BOOST_CHECK_EQUAL(d2.get_value(), 3);
}

BOOST_AUTO_TEST_CASE(DiceConstructionInvalid) {
    BOOST_CHECK_THROW(Dice(0), std::invalid_argument);
    BOOST_CHECK_THROW(Dice(7), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(DiceRoll) {
    Dice d;
    std::set<unsigned int> values;
    
    // Roll many times to ensure randomness
    for (int i = 0; i < 100; ++i) {
        d.roll();
        unsigned int value = d.get_value();
        BOOST_CHECK(value >= Dice::MIN_VALUE);
        BOOST_CHECK(value <= Dice::MAX_VALUE);
        values.insert(value);
    }
    
    // Should have seen multiple different values
    BOOST_CHECK(values.size() > 1);
}

BOOST_AUTO_TEST_CASE(DiceEquality) {
    Dice d1(4);
    Dice d2(4);
    Dice d3(5);
    
    BOOST_CHECK(d1 == d2);
    BOOST_CHECK(d1 != d3);
}

BOOST_AUTO_TEST_SUITE_END()