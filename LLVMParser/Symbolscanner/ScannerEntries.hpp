#pragma once
#include <string>
#include <vector>

#include "../../ASTParser/AST.hpp"
#include "../../ASTParser/Specifiers.hpp"
#include "llvm/IR/DerivedTypes.h"
struct SymboEntry {};

struct FunctionEntry  : virtual SymboEntry{

};

struct StaticVarEntry : virtual SymboEntry{
    bool isConst;
};

struct FieldEntry : virtual SymboEntry{
    const VarDefNode* target;
    std::vector<std::string> path;
    Vartype type;
    uint specifiers;

    FieldEntry(const VarDefNode* target, const std::vector<std::string>& path)
    : target(target), path(path), type(target->type), specifiers(target->specifiers)
    {
        this->path.push_back(target->name);
    }
};

struct StructEntry : virtual SymboEntry{
    const StructNode* target;
    StructEntry(const std::vector<std::string>& path, const StructNode* target)
    : path(path),target(target), fields(), dependencies(){
        this->path.push_back(target->name);
        FindFieldsAndDepends(target);
    };
    void FindFieldsAndDepends(const StructNode* target){
        for(auto* member : target->members){ 
            if(auto a = dynamic_cast<const VarDefNode*>(member)){
                if((a->specifiers & Specifiers::STATIC) == 0){
                    FieldEntry f = FieldEntry(a, path);
                    fields.push_back(f);
                    if(f.type.structPath.has_value()){
                        dependencies.push_back(f.type.structPath.value());
                    }
                }
            }
        }
    }
    
    std::vector<std::string> path;
    std::vector<SymboEntry> fields;
    std::vector<std::vector<std::string>> dependencies;
};