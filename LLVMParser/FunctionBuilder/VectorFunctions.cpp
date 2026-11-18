#include "FunctionBuilder.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Alignment.h"
void LLVMBuilder::FunctionBuilder::CreatePushBack(TypedValue vector, TypedValue v){
    llvm::BasicBlock* growBB =  llvm::BasicBlock::Create(parent.ctx, "push.grow",fn);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(parent.ctx, "push.end",fn);

    auto& b = parent.builder;
    
    // check if it needs more space
    auto dataPtrF = parent.GetStructField(vector, 0, path);
    auto sizeF = parent.GetStructField(vector, 1, path);
    auto capF = parent.GetStructField(vector, 2, path);
    
    llvm::Value* sizeVal = b.CreateLoad(GetLLVMType(sizeF.type),sizeF.value, "sizeVal");
    llvm::Value* isStringConst = b.CreateICmpSLT(sizeVal, b.getInt64(0));
    sizeVal = b.CreateAnd(sizeVal, b.getInt64(~STR_CONST_FLAG));
    llvm::Value* capVal = b.CreateLoad(GetLLVMType(capF.type),capF.value, "capVal");

    llvm::Value* capBytes = b.CreateMul(capVal, b.getInt64(GetVartypeSize(*vector.type.vectorType)));
    llvm::Value* needsSpace = b.CreateICmpEQ(sizeVal, capVal);
    needsSpace = b.CreateOr(needsSpace, isStringConst);
    // space += 1
    llvm::Value *sizeP1 = parent.builder.CreateAdd(sizeVal, parent.builder.getInt64(1), "spacePlusOne");
    parent.builder.CreateStore(sizeP1, sizeF.value);

    b.CreateCondBr(needsSpace, growBB,endBB);

    // grow
    b.SetInsertPoint(growBB);

    llvm::Value* newCap = b.CreateMul(capVal, b.getInt64(2),"newCap");
    llvm::Value *newAllocation = CreateMalloc({capBytes,PrimitiveType::ULONG});

    // allocate values
    b.CreateMemCpy(newAllocation,llvm::MaybeAlign(), dataPtrF.value, llvm::MaybeAlign(), capBytes);
    b.CreateStore(newCap, capF.value); // set new cap
    llvm::Value* dataPtr = b.CreateLoad(llvm::PointerType::getUnqual(parent.ctx),dataPtrF.value, "dataPtr");
    CreateFree({dataPtr,Vartype::NewPtr(PrimitiveType::VOID)});
    b.CreateStore(newAllocation,dataPtrF.value);
    b.CreateBr(endBB);

    b.SetInsertPoint(endBB);
}