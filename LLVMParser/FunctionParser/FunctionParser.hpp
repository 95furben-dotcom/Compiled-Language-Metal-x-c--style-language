#pragma once
#include "../LLVMModule.hpp"
struct FunctionEmiter{
    const Path& path;
    LLVMMoudle& llvmmodule;

    std::unordered_map<std::string, int> values;

    Vartype returnType;
    
    // if this is nullptr, then it is static
    // pointer to the parent is in [sp, #8]
    const FunctionEntry* objectEntry = nullptr;
    bool isStatic;
}