#pragma once
#include <optional>
#include <set>
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
        ComputeSizes();
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
            return entry.size;
        }
    }


    uint ComputeStructSize(const StructEntry& entry){
        // all depenencies MUST BE computed 
        uint size = 0;
        if(auto t = dynamic_cast<const StructNode*>(entry.target)){
            for(auto& d : t->members){
                if(auto defnode = dynamic_cast<const VarDefNode*>(d)){
                    // is struct: find size 
                    size += GetVartypeSize(defnode->type, entry.path);
                }
            }
            return size;
        }

        throw CodeGenError("Invalid target for struct");
    }
    
    void ComputeSizes() {
        std::set<std::string> resolved;
        
        // add primitive sizes first
        // keep looping until all resolved
        bool progress = true;
        while(progress){
            progress = false;
            for(auto& info : structEntries){
                std::string name = join(info.path, '.');
                if(resolved.contains(name)) continue;
                
                // check if all dependencies are resolved
                bool depsReady = true;
                for(auto& dep : info.dependencies){
                    auto depInfo =FindStruct(dep, info.path);
                    if(!depInfo || !resolved.contains(join(depInfo->path, '.'))){
                        depsReady = false;
                        break;
                    }
                }
                
                if(depsReady){
                    // compute size from fields
                    info.size = ComputeStructSize(info);
                    resolved.insert(name);
                    progress = true;
                }
            }
        }
        
        if(resolved.size() != structEntries.size())
            throw CodeGenError("Circular struct dependency detected");
    }


    private:
    void GenerateInfos(const NamespaceNode* program){
        structEntries = std::vector<StructEntry>();

        Path p{};
        for(auto*a : program->declarations){
            GenerateStructInfos(a, p);
        }
    }

    void GenerateFunctionInfos(const DeclNode* node, Path& path){
        if(auto a = TryCast<StructNode>(node)){
            path.push_back(a->name);
            GenerateStructInfos(a, path);
            path.pop_back();
        }
        if(auto a = TryCast<NamespaceNode>(node)){
            path.push_back(a->name);
            GenerateStructInfos(a, path);
            path.pop_back();
        }
        if(auto a = TryCast<FunctionNode>(node)){
            FunctionEntry e(a, path);
        }
    }

    void GenerateStructInfos(const DeclNode* node, Path& path){
        if(auto a = TryCast<StructNode>(node)){
            structEntries.push_back(StructEntry(a, path));

            path.push_back(a->name);
            GenerateStructInfos(a, path);
            path.pop_back();
        }
        if(auto a = TryCast<NamespaceNode>(node)){
            path.push_back(a->name);
            GenerateStructInfos(a, path);
            path.pop_back();
        }
    }
};