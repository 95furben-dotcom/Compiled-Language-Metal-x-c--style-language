#pragma once
#include <chrono>
#include <string>
#include <vector>

#include "../../ASTParser/AST.hpp"
#include "../../ASTParser/Specifiers.hpp"
#include "llvm/IR/DerivedTypes.h"
struct SymboEntry {
    const Node* _target;  
    std::vector<std::string> path;
    SymboEntry(const Node* target, const std::vector<std::string>& path, std::string name)
    : _target(target),path(path){
        this->path.push_back(name);
    }
};

struct FunctionEntry  : virtual SymboEntry{
    Vartype returnType;
    uint specifiers;
    std::vector<VarDefNode*> params;

    FunctionEntry(const std::vector<std::string>& path, const FunctionNode* target)
    : SymboEntry(target, path,target->name), returnType(target->returnType),specifiers(target->specifiers),params(target->params){};

};
 
struct StaticVarEntry : virtual SymboEntry{
    bool isConst;
};

struct FieldEntry : virtual SymboEntry{
    Vartype type;
    uint specifiers;

    FieldEntry(const VarDefNode* target, const std::vector<std::string>& path)
    : SymboEntry(target, path,target->name), type(target->type), specifiers(target->specifiers){}
};

struct StructEntry : virtual SymboEntry{
    std::vector<FieldEntry> fields;
    std::vector<std::vector<std::string>> dependencies;

    StructEntry(const StructNode* target, const std::vector<std::string>& path)
    : SymboEntry(target, path,target->name), fields(), dependencies(){
        FindFieldsAndDepends(target);
    };

    void FindFieldsAndDepends(const StructNode* target){
        for (MemberNode *member : target->members) {
            if (auto a = dynamic_cast<const VarDefNode *>(member)) {
                if ((a->specifiers & Specifiers::STATIC) == 0) {
                    FieldEntry f = FieldEntry(a, path);
                    fields.push_back(f);
                    if (f.type.structPath.has_value()) {
                        dependencies.push_back(f.type.structPath.value());
                    }
                }
            }
        }
    }
};