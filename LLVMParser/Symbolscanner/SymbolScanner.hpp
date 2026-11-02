#pragma once
#include <vector>

#include "ScannerEntries.hpp"
#include "../../ASTParser/AST.hpp"
struct Symbolscanner{
    std::vector<StructEntry> structEntries;
    std::vector<FunctionEntry> functionEntries;
    std::vector<FieldEntry> staticVariables;

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
            GenerateFunctionInfos(a, path);
            path.pop_back();
        }
        if(auto a = dynamic_cast<const NamespaceNode*>(node)){
            path.push_back(a->name);
            GenerateFunctionInfos(a, path);
            path.pop_back();
        }
        if(auto a = dynamic_cast<const FunctionNode*>(node)){
            FunctionEntry e(path,a);
            functionEntries.push_back(e);
        }

    }

    void GenerateStructInfos(const DeclNode* node, std::vector<std::string>& path){
        if(auto a = dynamic_cast<const StructNode*>(node)){
            structEntries.push_back(StructEntry(a,path));

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