#pragma once
#include <vector>

#include "ScannerEntries.hpp"
#include "../../ASTParser/AST.hpp"
struct Symbolscanner{
    std::vector<StructEntry> structEntries;
    std::vector<FunctionEntry> functionEntries;


    Symbolscanner(const NamespaceNode* program){
        GenerateInfos(program);
    }

    void GenerateInfos(const NamespaceNode* program){
        structEntries = std::vector<StructEntry>();

        std::vector<std::string> path = {};
        for(auto*a : program->declarations){
            GenerateStructInfos(a, path);
            GenerateFunctionInfos(a, path);
        }
    }

    void GenerateFunctionInfos(const DeclNode* node, std::vector<std::string>& path){
        if(auto a = dynamic_cast<const StructNode*>(node)){
            path.push_back(a->name);
            GenerateStructInfos(a, path);
            path.pop_back();
        }
        if(auto a = dynamic_cast<const NamespaceNode*>(node)){
            path.push_back(a->name);
            GenerateStructInfos(a, path);
            path.pop_back();
        }
        if(auto a = dynamic_cast<const FunctionNode*>(node)){
            
        }

    }

    private:
    void GenerateStructInfos(const DeclNode* node, std::vector<std::string>& path){
        if(auto a = dynamic_cast<const StructNode*>(node)){
            structEntries.push_back(StructEntry(path, a));

            path.push_back(a->name);
            GenerateStructInfos(a, path);
            path.pop_back();
        }
        if(auto a = dynamic_cast<const NamespaceNode*>(node)){
            path.push_back(a->name);
            GenerateStructInfos(a, path);
            path.pop_back();
        }
    }
};