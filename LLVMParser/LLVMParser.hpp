#pragma once
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

#include "../ASTParser/AST.hpp"
#include "Symbolscanner/SymbolScanner.hpp"

#include "Utillity/Caster.hpp"

struct LLVMParser{
    llvm::LLVMContext ctx;
    llvm::Module mod = llvm::Module("program", ctx);
    llvm::IRBuilder<> builder = llvm::IRBuilder<>(ctx);

    SymbolScanner symbolScanner;

    LLVMParser(const NamespaceNode* program)
    : symbolScanner(program){

    }

    static void ASTToLLVM(const NamespaceNode* program){
        LLVMParser parser(program);
        parser.BuildNamespace(program);
    }

    void BuildNamespace(const NamespaceNode* namespaceNode, std::vector<std::string> path = {});

    void BuildStruct(const StructNode* structNode, std::vector<std::string> path = {});

    struct FunctionBuilder;
};