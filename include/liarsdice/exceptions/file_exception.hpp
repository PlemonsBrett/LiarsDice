//
// Created by Brett on 10/14/2023.
//

#ifndef LIARSDICE_INCLUDE_EXCEPTIONS_FILEEXCEPTION_HPP
#define LIARSDICE_INCLUDE_EXCEPTIONS_FILEEXCEPTION_HPP

#include "liarsdice/exceptions/exception_base.hpp"

class FileException : public CustomException {
public:
  explicit FileException(const std::string &message) : CustomException("File Error: " + message) {}
};

#endif // LIARSDICE_INCLUDE_EXCEPTIONS_FILEEXCEPTION_HPP
