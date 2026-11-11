#include "LLVMBuilder.hpp"

llvm::Function* LLVMBuilder::mallocFn = nullptr;
llvm::Function* LLVMBuilder::freeFn   = nullptr;
llvm::Function* LLVMBuilder::printfFn = nullptr;


void LLVMBuilder::DeclareLibcFunctions(){
    llvm::Type* ptrTy = llvm::PointerType::get(ctx, 0);

    mallocFn = llvm::Function::Create(
        llvm::FunctionType::get(ptrTy, {builder.getInt64Ty()}, false),
        llvm::Function::ExternalLinkage, "malloc", mod);
    
    freeFn = llvm::Function::Create(
        llvm::FunctionType::get(builder.getVoidTy(), {ptrTy}, false),
        llvm::Function::ExternalLinkage, "free", mod);
    
    printfFn = llvm::Function::Create(
        llvm::FunctionType::get(builder.getInt32Ty(), {ptrTy}, /*isVarArg=*/true),
        llvm::Function::ExternalLinkage, "printf", mod);
}