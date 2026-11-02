#pragma once
#include <exception>
#include <vector>
inline std::vector<std::string> codeGenCallStack;
inline std::string GetStackInfo(){
    std::string trace = ""; \
    for(auto& s : codeGenCallStack) trace += "\n  in " + s; \
    return trace;
}

struct StackGuard {
    std::vector<std::string>& stack;
    StackGuard(std::vector<std::string>& s, std::string name) : stack(s) {
        stack.push_back(name);
    }
    ~StackGuard() { stack.pop_back(); }
};

#define CALLSTACKADD() StackGuard _guard_(codeGenCallStack, \
    std::string(__func__) + "()" + "\t\t" + __FILE__ + ":" + std::to_string(__LINE__));


class CodeGenError : public std::exception {
    std::string message;
    

public:
    CodeGenError(const std::string& msg)
        : message(GetStackInfo()+"\nerror: "+msg) {}

    const char* what() const noexcept override {
        
        return message.c_str();
    }
};

using Path = std::vector<std::string>;