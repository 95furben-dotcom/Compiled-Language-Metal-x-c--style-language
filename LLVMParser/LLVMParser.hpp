#pragma once
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

#include "../ASTParser/AST.hpp"
#include "Symbolscanner/SymbolScanner.hpp"

struct LLVMParser{
llvm::LLVMContext ctx;
llvm::Module mod = llvm::Module("program", ctx);
llvm::IRBuilder<> builder = llvm::IRBuilder<>(ctx);

Symbolscanner symbolScanner;

LLVMParser(const NamespaceNode* program)
: symbolScanner(program)
{}

static void ASTToLLVM(const NamespaceNode* program){
    std::vector<std::string> path;
    LLVMParser parser(program);
    parser.BuildNamespace(program, path);
}

void BuildNamespace(const NamespaceNode* program, std::vector<std::string> path){
    if(program->name != "program"){
        path.push_back(program->name);
    }
    for (const DeclNode *decl : program->declarations) {
        if()
    }
}

void BuldFunction(const FunctionNode* function, std::vector<std::string> path){

}
}