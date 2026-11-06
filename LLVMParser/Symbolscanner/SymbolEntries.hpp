#pragma once
#include <cstddef>
#include <string>
#include <vector>

#include "../../ASTParser/AST.hpp"
#include "../../ASTParser/Specifiers.hpp"
#include "../Utillity/Caster.hpp"
#include "llvm/IR/Type.h"

struct SymbolEntry{
    const Node *target;
    Path path;
    SymbolEntry(const Node* target, const Path& path, std::string name) : target(target), path(path) {
        this->path.push_back(name);
    }
    virtual ~SymbolEntry() = default;   // <-- add this

};

struct FunctionEntry : virtual SymbolEntry{
    FunctionEntry(const FunctionNode *target,const Path &path)
      : SymbolEntry(target, path, target->name), returnType(std::move(target->returnType)),
        parameters(target->params), specifiers(target->specifiers) {}
    Vartype returnType;
    std::vector<VarDefNode*> parameters;

  uint specifiers;
};

struct VariableEntry : SymbolEntry{
    Vartype type;
    uint specifiers;

    VariableEntry(const VarDefNode* target, const Path& path)
    : SymbolEntry(target, path, target->name), type(target->type), specifiers(target->specifiers){}

    const VarDefNode* GetTarget() const {return Cast<VarDefNode>(target);}
};

struct StructEntry : SymbolEntry{
    std::vector<VariableEntry> fields;
    std::vector<Path> dependencies;

    llvm::Type* llvmType = nullptr;

    uint size; // bytes
    StructEntry(const StructNode* target, const Path& path)
    : SymbolEntry(target, path, target->name), fields(), dependencies(){
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

struct KernelEntry : virtual SymbolEntry{
    KernelEntry(const KernelDefinitionNode *target,const Path &path)
      : SymbolEntry(target, path, target->name),
        parameters(target->params) {}
    std::vector<VarDefNode*> parameters;
};