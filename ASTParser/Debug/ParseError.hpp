#pragma once
#include <expected>
#include <source_location>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>

// for Result<T> where T is a real value you want back
#define TRY(expr) ({ \
    auto _r = addFrame(expr); \
    if (!_r) return std::unexpected(_r.error()); \
    std::move(_r).value(); \
})

// for Result<void> — check and propagate, nothing to extract
#define TRY_VOID(expr) do { \
    auto _r = addFrame(expr); \
    if (!_r) return std::unexpected(_r.error()); \
} while(0)



struct Frame { std::string func; const char* file; int line; };

struct ParseError {
  ParseError(const std::string& message, uint tokenIndex, const std::vector<Frame>& trace)
      : message(message), tokenIndex(tokenIndex),
        trace(trace) {        }
  std::string message;
  uint tokenIndex;
  std::vector<Frame> trace; // built lazily, only on the failure path

};

template<typename T>
using Result = std::expected<T, ParseError>;

inline std::unexpected<ParseError> Fail(std::string msg, uint tokenIndex, std::source_location loc = std::source_location::current()) {
    if (msg.empty())
        throw std::runtime_error(
            std::string("Fail() called with empty message at ")
            + loc.file_name() + ":" + std::to_string(loc.line()));
    return std::unexpected(ParseError(msg, tokenIndex, {}));
}

template<typename T>
Result<T> addFrame(Result<T> r, std::source_location loc = std::source_location::current()) {
    if (!r) r.error().trace.push_back({loc.function_name(), loc.file_name(), (int)loc.line()});
    return r;
}