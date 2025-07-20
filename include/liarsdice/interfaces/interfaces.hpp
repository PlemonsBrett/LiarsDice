#pragma once

// Main interfaces header - includes all interface definitions
#include "i_dice.hpp"
#include "i_random_generator.hpp"
#include "i_player.hpp"
#include "i_game_state.hpp"
#include "i_game.hpp"

namespace liarsdice::interfaces {

// Type aliases for common interface types
using DicePtr = std::unique_ptr<IDice>;
using PlayerPtr = std::unique_ptr<IPlayer>;
using GameStatePtr = std::unique_ptr<IGameState>;
using GamePtr = std::unique_ptr<IGame>;
using RandomGeneratorPtr = std::unique_ptr<IRandomGenerator>;

} // namespace liarsdice::interfaces