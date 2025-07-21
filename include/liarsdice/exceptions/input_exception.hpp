//
// Created by Brett on 10/14/2023.
//

#ifndef LIARSDICE_INCLUDE_EXCEPTIONS_INPUTEXCEPTION_HPP
#define LIARSDICE_INCLUDE_EXCEPTIONS_INPUTEXCEPTION_HPP

#include "liarsdice/exceptions/exception_base.hpp"

class InputException : public CustomException {
public:
  explicit InputException(const std::string &message)
      : CustomException("Input Error: " + message) {}
};

#endif // LIARSDICE_INCLUDE_EXCEPTIONS_INPUTEXCEPTION_HPP
