#include "LLVMBuilder.hpp"
#include "FunctionBuilder/FunctionBuilder.hpp"
#include "Utillity/Caster.hpp"
#include "Utillity/LLVMDebug.hpp"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"

void LLVMBuilder::BuildNamespace(const NamespaceNode* namespaceNode, std::vector<std::string> path){
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
            path.push_back(n->name);
            FunctionBuilder f(n, path, *this);
            f.BuildFunction();
            path.pop_back();
        }
    }
}

void LLVMBuilder::BuildStaticVariables() {
    for (const auto& v : symbolScanner.staticVariableEntries) {
        llvm::Type* ty = GetLLVMType(v.type,v.path);
        bool isConstant = (Specifiers::CONST & v.specifiers) != 0;

        const auto* def = Cast<VarDefNode>(v.GetTarget());
        llvm::Constant* init = def->value
            ? BuildConstant(def->value, v.type,v.path)
            : llvm::Constant::getNullValue(ty);

        auto* gv = new llvm::GlobalVariable(
            mod, ty, isConstant, llvm::GlobalValue::ExternalLinkage,
            init, join(v.path, '.'));

        globals.insert_or_assign(join(v.path, '.'), GlobalVar{ gv, ty, v.type });
        printf("Added Global %s",join(v.path, '.').c_str());
    }
}

void LLVMBuilder::BuildStruct(const StructNode* structNode, std::vector<std::string> path){
    for (const MemberNode *member : structNode->members) {
        if (auto *n = TryCast<StructNode>(member)) {
            path.push_back(n->name);
            BuildStruct(n, path);
            path.pop_back();
        }
        if(auto* n = TryCast<FunctionNode>(member)){
            path.push_back(n->name);
            FunctionBuilder f(n, path, *this);
            f.BuildFunction();
            path.pop_back();

        }
    }
}



void LLVMBuilder::WriteToFile(const std::string& filepath) {
    // check the IR is valid first; this catches type mismatches, missing terminators, etc.
    if (llvm::verifyModule(mod, &llvm::errs()))
        throw CodeGenError("LLVM module verification failed");

    std::error_code ec;
    llvm::raw_fd_ostream out(filepath, ec, llvm::sys::fs::OF_Text);
    if (ec)
        throw CodeGenError("Could not open " + filepath + ": " + ec.message());

    mod.print(out, nullptr);
}

LLVMBuilder::GlobalVar* LLVMBuilder::FindGloabal(const Path& targetPath){
    auto p = targetPath;

    while(p.size()>=1){
        std::string label = join(p,'.');
        // printf("Searching for label: %s\n",label.c_str());
        if(globals.contains(label))
            return &globals.at(label);
        p.pop_back();
    }
    return nullptr;
}

void LLVMBuilder::DeclareFunctions() {
    for (const auto& f : symbolScanner.functionEntries) {
        std::vector<llvm::Type*> paramTypes;
        for (auto* p : f.parameters)
            paramTypes.push_back(GetLLVMType(p->type, f.path));

        llvm::Type* ret = GetLLVMType(f.returnType, f.path);
        auto* fnType = llvm::FunctionType::get(ret, paramTypes, false);

        llvm::Function::Create(fnType, llvm::Function::ExternalLinkage,
                               join(f.path, '.'), mod);
    }
}