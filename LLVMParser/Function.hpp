#pragma once
#include "LLVMParser.hpp"
#include "llvmParser.hpp"
#include "llvm/IR/Value.h"
namespace LLVMParser{
static void ParseFuction(const FunctionNode* function){
    
}

static void CreateVarDef(const VarDefNode* node){
    builder.CreateAlloca(builder.getInt32Ty());

    
}
}