#pragma once
#include "../Vartype.hpp"
#include <string>
#include <sys/types.h>

struct Token{
    virtual ~Token() = default;
    uint lineIndex = 0;
    uint charIndex = 0;  
};

struct signToken : virtual Token{
    signToken(char c) : c(c){}
    char c;
}; 
struct typeToken : virtual Token{
    typeToken(Vartype type) : type(type){}
    Vartype type;
}; 
struct operatorToken : virtual Token{
    operatorToken(std::string s) : s(s){}
    std::string s;
};
struct assignmentToken : virtual Token{
    assignmentToken(char op) : c(op){}
    char c; // +, -, *, /, =
};
struct stringToken : virtual Token{
    stringToken(std::string s) : s(s){}
    std::string s;
};
struct endlToken : virtual Token{};
struct returnToken : virtual Token{};
struct valueToken : virtual Token{
    valueToken(std::string s) : s(s){}
    std::string s;
};
struct definisionToken : virtual Token{
    definisionToken(std::string name) : name(name){}
    std::string name;
};

struct structToken : virtual Token{};

struct namespaceToken : virtual Token{};

struct specifierToken : virtual Token{
    specifierToken(std::string s):s(s){};
    std::string s;
};

struct iterateToken : virtual Token{
    iterateToken(bool isPluss):isPluss(isPluss){};
    bool isPluss;
};

struct ifToken:       virtual Token {};
struct elseToken:     virtual Token {};
struct whileToken:    virtual Token {};
struct forToken:      virtual Token {};
struct continueToken: virtual Token {};
struct breakToken:    virtual Token {};