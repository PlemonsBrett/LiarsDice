#define BOOST_TEST_MODULE AITests
#include <boost/test/unit_test.hpp>
#include <liarsdice/ai/ai_player.hpp>

using namespace liarsdice::ai;
using namespace liarsdice::core;

BOOST_AUTO_TEST_SUITE(AITestSuite)

BOOST_AUTO_TEST_CASE(AIPlayerConstruction) {
    EasyAI easy(1);
    BOOST_CHECK_EQUAL(easy.get_id(), 1);
    BOOST_CHECK_EQUAL(easy.get_name(), "Easy AI 1");
    
    MediumAI medium(2);
    BOOST_CHECK_EQUAL(medium.get_id(), 2);
    BOOST_CHECK_EQUAL(medium.get_name(), "Medium AI 2");
    
    HardAI hard(3);
    BOOST_CHECK_EQUAL(hard.get_id(), 3);
    BOOST_CHECK_EQUAL(hard.get_name(), "Hard AI 3");
}

BOOST_AUTO_TEST_CASE(AIDecisionMaking) {
    EasyAI ai(1);
    ai.roll_dice();
    
    // Test first guess (no previous guess)
    auto guess = ai.make_guess(std::nullopt);
    BOOST_CHECK(guess.dice_count > 0);
    BOOST_CHECK(guess.face_value >= 1 && guess.face_value <= 6);
    BOOST_CHECK_EQUAL(guess.player_id, 1);
    
    // Test response to previous guess
    Guess prev_guess{3, 4, 2}; // 3 fours from player 2
    auto response = ai.make_guess(prev_guess);
    
    // Response should be valid (higher than previous)
    bool valid = (response.face_value > prev_guess.face_value) ||
                 (response.face_value == prev_guess.face_value && 
                  response.dice_count > prev_guess.dice_count) ||
                 (response.face_value < prev_guess.face_value && 
                  response.dice_count > prev_guess.dice_count);
    BOOST_CHECK(valid);
}

BOOST_AUTO_TEST_SUITE_END()