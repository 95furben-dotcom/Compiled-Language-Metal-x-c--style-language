#include "../LLVMBuilder.hpp"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include <cstddef>
#include "FunctionBuilder.hpp"
using fb = LLVMBuilder::FunctionBuilder;
void fb::BuildFunction(){
    std::vector<llvm::Type*> paramTypes = {};
    for(auto p : functionNode->params){
        llvm::Type* t = GetLLVMType(p->type);
        paramTypes.push_back(t);
    }

    llvm::FunctionType* fnType = llvm::FunctionType::get(
        GetLLVMType(functionNode->returnType), 
        paramTypes, 
        false
    );

    fn = parent.mod.getFunction(join(path,'.'));

    entry = llvm::BasicBlock::Create(parent.ctx, "entry", fn);
    parent.builder.SetInsertPoint(entry);

    for(int i = 0; i < functionNode->params.size(); i++){
        auto p = functionNode->params[i];
        llvm::Type* t = GetLLVMType(p->type);
        LocalVar l(p->type,parent.builder.CreateAlloca(t,nullptr, p->name));
        llvm::Argument* arg = fn->getArg(i);
        parent.builder.CreateStore(arg, l.AllocaInst);
        locals.insert_or_assign(p->name, l);
    }
 
    for(auto statement : functionNode->body->statements){
        BuildStatement(statement);
    }
}


LLVMBuilder::TypedValue fb::BuildValue(const ExprNode* node){
    if (const VariableCallNode *n = TryCast<VariableCallNode>(node)) {
        if(auto v = GetLocalVar(n->name)){
            llvm::Type* ty = parent.GetLLVMType(v->type, path);
            return { parent.builder.CreateLoad(ty, v->AllocaInst, n->name), v->type };
        }
    }
    if(auto op = TryCast<OprandNode>(node)){
        auto a = BuildValue(op->v1);
        auto b = BuildValue(op->v2);
        return parent.BuildOprand(a, b, op->oprand, path);
    }
    if(auto f = TryCast<FuncCallNode>(node)){
        return CreateFuncCall(f);
    }
    return parent.BuildValue(node, path);
}

const fb::LocalVar* fb::GetLocalVar(std::string target) {
    for (const auto& pair : locals) {
        if (pair.first == target) {
            return &pair.second; 
        }
    }
    return nullptr;
}

