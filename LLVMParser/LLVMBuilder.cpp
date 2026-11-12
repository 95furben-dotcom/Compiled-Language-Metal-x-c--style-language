#include "LLVMBuilder.hpp"
#include "FunctionBuilder/FunctionBuilder.hpp"
#include "Utillity/Caster.hpp"
#include "Utillity/LLVMDebug.hpp"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"

#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/IR/LegacyPassManager.h"


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

void LLVMBuilder::DeclareStructTypes() {
    // pass 1: opaque types, so order and recursion don't matter
    for (auto& s : symbolScanner.structEntries)
        structTypes[join(s.path, '.')] = llvm::StructType::create(ctx, join(s.path, '.'));

    // pass 2: bodies. A by-value field needs its dependency's body set first,
    // so process in dependency order (your SymbolScanner already sorts for sizes).
    for (auto& s : symbolScanner.structEntries) {
        std::vector<llvm::Type*> fields;
        for (auto& f : s.fields)
            fields.push_back(GetLLVMType(f.type, s.path));
        structTypes[join(s.path, '.')]->setBody(fields);
    }
}

void LLVMBuilder::WriteObjectFile(const std::string& path) {
    if (llvm::verifyModule(mod, &llvm::errs()))
        throw CodeGenError("LLVM module verification failed");

    std::error_code ec;
    llvm::raw_fd_ostream out(path, ec, llvm::sys::fs::OF_None);   // binary, not OF_Text
    if (ec) throw CodeGenError("Could not open " + path + ": " + ec.message());

    llvm::legacy::PassManager pm;
    if (targetMachine->addPassesToEmitFile(pm, out, nullptr,
                                           llvm::CodeGenFileType::ObjectFile))
                                           
        throw CodeGenError("Target can't emit object files");
    pm.run(mod);
    out.flush();
}

void LLVMBuilder::InitTarget() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    llvm::Triple triple(llvm::sys::getDefaultTargetTriple());
    std::string err;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple, err);
    if (!target) throw CodeGenError("Target lookup failed: " + err);

    llvm::TargetOptions opts;
    targetMachine.reset(target->createTargetMachine(
        triple, "generic", "", opts, llvm::Reloc::PIC_));

    mod.setTargetTriple(triple);
    mod.setDataLayout(targetMachine->createDataLayout());
}