#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include "../../ASTParser/AST.hpp"
#include "../../ASTParser/Specifiers.hpp"
#include "../Utillity/Caster.hpp"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

#include "../CodeGenError.hpp"

struct FunctionEntry {
    FunctionEntry(const Path &path, const Node *target,
                Vartype returnType, const std::vector<Vartype> &parameters,
                uint specifiers)
      : path(path), target(target), returnType(std::move(returnType)),
        parameters(parameters), specifiers(specifiers) {}

    Path path;
    const Node *target;
    Vartype returnType;
    std::vector<Vartype> parameters;

  uint specifiers;
};

struct VariableEntry{
    const VarDefNode* target;
    Path path;
    Vartype type;
    uint specifiers;

    VariableEntry(const VarDefNode* target, const Path& path)
    : target(target), path(path), type(target->type), specifiers(target->specifiers)
    {
        this->path.push_back(target->name);
    }
};

struct StructEntry {
    Path path;
    std::vector<VariableEntry> fields;
    std::vector<Path> dependencies;

    llvm::Type* llvmType = nullptr;
    const StructNode* target;
    StructEntry(const Path& path, const StructNode* target)
    : path(path),target(target), fields(), dependencies(){
        this->path.push_back(target->name);
        FindFieldsAndDepends(target);
    };

    void FindFieldsAndDepends(const StructNode* target){
        for(auto* member : target->members){ 
            if(auto a = TryCast<VarDefNode>(member)){
                if((a->specifiers & Specifiers::STATIC) == 0){
                    VariableEntry f = VariableEntry(a, path);
                    fields.push_back(f);
                    if(f.type.structPath.has_value()){
                        dependencies.push_back(f.type.structPath.value());
                    }
                }
            }
        }
    }
};