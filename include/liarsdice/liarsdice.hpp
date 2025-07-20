#pragma once

// LiarsDice Library - Main include file
// Include this header to access all public APIs

// Core game components
#include "liarsdice/core/dice.hpp"
#include "liarsdice/core/game.hpp"
#include "liarsdice/core/player.hpp"

// Exception hierarchy
#include "liarsdice/exceptions/exception_base.hpp"
#include "liarsdice/exceptions/file_exception.hpp"
#include "liarsdice/exceptions/game_logic_exception.hpp"
#include "liarsdice/exceptions/input_exception.hpp"

// Dependency Injection system
#include "liarsdice/di/di.hpp"
#include "liarsdice/interfaces/interfaces.hpp"

// Adapters for legacy compatibility
#include "liarsdice/adapters/dice_adapter.hpp"
#include "liarsdice/adapters/random_generator.hpp"
