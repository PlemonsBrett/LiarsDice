//
// Created by Brett on 10/14/2023.
//

#ifndef LIARSDICE_INCLUDE_EXCEPTIONS_GAMELOGICEXCEPTION_HPP
#define LIARSDICE_INCLUDE_EXCEPTIONS_GAMELOGICEXCEPTION_HPP

#include "liarsdice/exceptions/exception_base.hpp"

class GameLogicException : public CustomException {
public:
  explicit GameLogicException(const std::string& message) : CustomException("Game Logic Error: " + message) {}
};

#endif //LIARSDICE_INCLUDE_EXCEPTIONS_GAMELOGICEXCEPTION_HPP
