//
// Created by Brett on 10/14/2023.
//

#ifndef LIARSDICE_INCLUDE_EXCEPTIONS_CUSTOMEXCEPTION_HPP
#define LIARSDICE_INCLUDE_EXCEPTIONS_CUSTOMEXCEPTION_HPP

#include <string>
#include <utility>

class CustomException : public std::exception {
public:
  explicit CustomException(std::string  message) : message_(std::move(message)) {}
  [[nodiscard]] const char* what() const noexcept override {
    return message_.c_str();
  }
private:
  std::string message_;
};

#endif //LIARSDICE_INCLUDE_EXCEPTIONS_CUSTOMEXCEPTION_HPP
