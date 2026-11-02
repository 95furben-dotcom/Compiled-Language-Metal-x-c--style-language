#pragma once
#include <optional>
#include <vector>

#include "ScannerEntries.hpp"
#include "../../ASTParser/AST.hpp"

#include "../Utillity/Caster.hpp"
struct Symbolscanner{
    std::vector<StructEntry> structEntries;
    std::vector<FunctionEntry> functionEntries;
    std::vector<VariableEntry> staticVariableEntries;


    Symbolscanner(const NamespaceNode* program){
        GenerateInfos(program);
    }

    std::optional<StructEntry> FindStruct(const Path& dep, Path scope) const{
        while(true){
            Path candidate = scope;
            candidate.insert(candidate.end(), dep.begin(), dep.end());

            for(uint i = 0; i < structEntries.size(); i++){
                if(structEntries[i].path == candidate)
                    return structEntries[i];
            }

            if(scope.empty())
                break;
            scope.pop_back(); // fallback: widen search by dropping the innermost scope level
        }
        return {};
    }

    std::optional<FunctionEntry> FindFunction(const Path& dep, Path scope) const{
        while(true){
            Path candidate = scope;
            candidate.insert(candidate.end(), dep.begin(), dep.end());
            for(uint i = 0; i < functionEntries.size(); i++){
                if(functionEntries[i].path == candidate)
                    return functionEntries[i];
            }

            if(scope.empty())
                break;
            scope.pop_back(); // fallback: widen search by dropping the innermost scope level
        }
        return {};
    }

    std::optional<VariableEntry> FindStaticVar(const Path& dep, Path scope) const{
        while(true){
            Path candidate = scope;
            candidate.insert(candidate.end(), dep.begin(), dep.end());
            for(uint i = 0; i < staticVariableEntries.size(); i++){
                if(staticVariableEntries[i].path == candidate)
                    return staticVariableEntries[i];
            }

            if(scope.empty())
                break;
            scope.pop_back(); // fallback: widen search by dropping the innermost scope level
        }
        return {};
    }



    uint GetVartypeSize(Vartype t, Path scope){
        if(t.IsPrimitive())
            return PrimitiveSize(t.primitivetype.value());
        else if(t.vectorType){
            return 24;
        }else{
            StructEntry entry = FindStruct(t.structPath.value(), scope).value();

            uint sum = 0;
            for(auto& f : entry.fields){
                sum += GetVartypeSize(f.type, entry.path);
            }
            for(Path& f : entry.dependencies){
                sum += GetVartypeSize(f.type, entry.path);
            }
        }
    }

    private:
    void GenerateInfos(const NamespaceNode* program){
        structEntries = std::vector<StructEntry>();

        Path path = {};
        for(auto*a : program->declarations){
            GenerateStructInfos(a, path);
        }
    }

    void GenerateFunctionInfo(const FunctionNode* node, Path& path){
        if(auto a = TryCast<FunctionNode>(node)){
            path.push_back(a->name);
            GenerateStructInfos(a, path);
            path.pop_back();
        }
        if(auto a = TryCast<FunctionNode>(node)){
            path.push_back(a->name);
            GenerateStructInfos(a, path);
            path.pop_back();
        }
        if(auto a = TryCast<FunctionNode>(node)){
            
        }

    }

    void GenerateStructInfos(const DeclNode* node, Path& path){
        if(auto a = TryCast< StructNode>(node)){
            structEntries.push_back(StructEntry(path, a));

            path.push_back(a->name);
            GenerateStructInfos(a, path);
            path.pop_back();
        }
        if(auto a = TryCast< NamespaceNode>(node)){
            path.push_back(a->name);
            GenerateStructInfos(a, path);
            path.pop_back();
        }
    }
};