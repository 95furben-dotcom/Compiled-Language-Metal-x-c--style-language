#pragma once
#include "../AST.hpp"
#include "ParseError.hpp"
#include <expected>
#include <iostream>
#include <source_location>
#include <string>
#include <type_traits>
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

#define ADVANCE()\
    do{\
    index++;\
    if(index>= tokens.size())\
        return Fail("Index bigger than tokensize", index); \
    std::cout << "Indexing at " << std::string(__func__) + "():" + std::to_string(index-1)+"\t\t" + __FILE__ + ":" + std::to_string(__LINE__) << "\n"; \
    } while(0)


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
T* Ensure(Result<T*> node){
    static_assert(std::is_base_of<Node, T>(),"T must be a node");
    T* errorNode = new T();
    errorNode->error = new ParseError(node.error());
    return errorNode;
}

template<typename T>
Result<T> addFrame(Result<T> r, std::source_location loc = std::source_location::current()) {
    if (!r) r.error().trace.push_back({loc.function_name(), loc.file_name(), (int)loc.line()});
    return r;
}


