#pragma once
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>

struct Frame { std::string func; const char* file; int line; };

struct ParseError {
  ParseError(const std::string& message, uint tokenIndex, const std::vector<Frame>& trace)
      : message(message), tokenIndex(tokenIndex),
        trace(trace) {        }
  std::string message;
  uint tokenIndex;
  std::vector<Frame> trace; // built lazily, only on the failure path

};