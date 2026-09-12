#pragma once
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

#include "../ASTParser/AST.hpp"

namespace LLVMParser{
static llvm::LLVMContext ctx;
static llvm::Module mod("program", ctx);
static llvm::IRBuilder<> builder(ctx);

static auto ASTToLLVM(const NamespaceNode* program){

}


}