#include "LLVMParser.hpp"
#include "FunctionBuilder/FunctionBuilder.hpp"

void LLVMParser::BuildNamespace(const NamespaceNode* namespaceNode, std::vector<std::string> path){
    for (const DeclNode *decl : namespaceNode->declarations) {
        if(auto* n = TryCast<NamespaceNode>(decl)){
            path.push_back(n->name);
            BuildNamespace(n,path);
            path.pop_back();
        }
        if(auto* n = TryCast<StructNode>(decl)){
            path.push_back(n->name);
            BuildStruct(n,path);
            path.pop_back();
        }
        if(auto* n = TryCast<FunctionNode>(decl)){
            FunctionBuilder f(n, path, *this);
            f.Build();
        }
    }
}


void LLVMParser::BuildStruct(const StructNode* structNode, std::vector<std::string> path){
    for (const MemberNode *decl : structNode->members) {
        if (auto *n = TryCast<NamespaceNode>(decl)) {
            path.push_back(n->name);
            BuildNamespace(n, path);
            path.pop_back();
        }
        if (auto *n = TryCast<StructNode>(decl)) {
            path.push_back(n->name);
            BuildNamespace(n, path);
            path.pop_back();
        }
    }
}