#include "FunctionBuilder.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include <vector>
void LLVMBuilder::FunctionBuilder::BuildStatement(const StatementNode* statement){
    if(auto n =TryCast<VarDefNode>(statement)){
        if((n->specifiers & Specifiers::STATIC) != 0)
            return;
        // local
        llvm::Type* ty = GetLLVMType(n->type);
        llvm::AllocaInst* alloca = parent.builder.CreateAlloca(ty, nullptr, n->name);
            
        llvm::Value* init;
        if(n->value){
            TypedValue v = BuildValue(n->value);
            init = parent.CastTo(v, n->type, path).value;
        }
        else{
            init = llvm::Constant::getNullValue(ty);            
        }
        parent.builder.CreateStore(init, alloca); 
        SetLocal(n->name, LocalVar{n->type, alloca});
        return;
    }
    if(auto n =TryCast<ReturnNode>(statement)){
        parent.builder.CreateRet(BuildValue(n->value).value);
        return;
    }
    if(auto n =TryCast<AssignNode>(statement)){
        auto target = GetAlloca(n->target);
        auto value = BuildValue(n->value);

        
        if(n->op != '='){
            llvm::Type* ty = parent.GetLLVMType(target.type, path);
            TypedValue current = { parent.builder.CreateLoad(ty, target.value), target.type };
            value = BuildOprand(current, value, {n->op});
        }
        value = parent.CastTo(value, target.type, path);
        parent.builder.CreateStore(value.value, target.value);
        return;
    }
    if(auto n =TryCast<FuncCallNode>(statement)){
        CreateFuncCall(n);
    }
    if(auto n =TryCast<IfNode>(statement)){
        PushScope();
        
    
        // true
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(parent.ctx, "iftrue", fn);
        llvm::BasicBlock *elseBB = n->elseBody ? llvm::BasicBlock::Create(parent.ctx, "iffalse", fn) : nullptr;
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(parent.ctx, "mergeBB", fn);
        
        TypedValue condition = BuildValue(n->condition);
        if(!condition.type.IsPrimitive() || condition.type.primitivetype.value() != PrimitiveType::BOOL)
            throw CodeGenError("Condition is not type bool");
        parent.builder.CreateCondBr(condition.value, thenBB, elseBB ? elseBB : mergeBB);
        
        parent.builder.SetInsertPoint(thenBB);
        for(auto statement : n->body->statements){
            BuildStatement(statement);
        }
        if (!parent.builder.GetInsertBlock()->getTerminator()) parent.builder.CreateBr(mergeBB);

        if (elseBB) {
            parent.builder.SetInsertPoint(elseBB);  
            for(auto statement : n->elseBody->statements){
                BuildStatement(statement);
            }
            if (!parent.builder.GetInsertBlock()->getTerminator()) parent.builder.CreateBr(mergeBB);
        }
        parent.builder.SetInsertPoint(mergeBB);
    return;
    }
    if(auto n =TryCast<WhileNode>(statement)){

    }
    if(auto n =TryCast<ForNode>(statement)){

    }
    if(auto n =TryCast<ContinueNode>(statement)){

    }
    if(auto n =TryCast<BreakNode>(statement)){

    }
    throw CodeGenError("StatementNode variation Not implimented");
}

LLVMBuilder::TypedValue LLVMBuilder::FunctionBuilder::GetAlloca(const ExprNode* node){
    if(auto n = TryCast<OprandNode>(node)){
        if(n->oprand == "[]")
            throw CodeGenError("Indexing Oprand Not Implimented");

        throw CodeGenError("GetAlloca ExprNode-variation OprandNode uncounted for");
    }
    if(auto n = TryCast<VariableCallNode>(node)){
        if(HasLocal(n->name)){
          const LocalVar *v = GetLocalVar(n->name);
          return {v->AllocaInst, v->type};
        }
        auto varPath = path;
        varPath.push_back(n->name);
        GlobalVar *g = parent.FindGloabal(varPath);
        if(g)
            return {g->global,g->type};

        throw CodeGenError("No global or local with name: " + n->name);
        
    }

    if(auto n = TryCast<StaticMemberNode>(node)){
        auto varPath = n->path;
        GlobalVar *g = parent.FindGloabal(varPath);
        if(g)
            return {g->global,g->type};

        throw CodeGenError("No global or local with path: " + join(n->path,'.'));
    }

    throw CodeGenError("Can not get allocator for this type of node");

    if(auto n = TryCast<FuncCallNode>(node)){
        throw CodeGenError("GetAlloca ExprNode-variation FuncCallNode uncounted for");
    }
    if(auto n = TryCast<InstanceMemberNode>(node)){
        throw CodeGenError("GetAlloca ExprNode-variation InstanceMemberNode uncounted for");
    }

}


LLVMBuilder::TypedValue LLVMBuilder::FunctionBuilder::CreateFuncCall(const FuncCallNode* n){
    auto params = n->params->params;
    std::vector<llvm::Value*> args;
    auto e = parent.symbolScanner.FindFunction({n->funcname}, path);
    if(!e) throw CodeGenError("Could not find function"+n->funcname);
    if(params.size() != e->parameters.size()) throw CodeGenError("Mismach in number of function parameters");
    llvm::Function* callee = parent.mod.getFunction(join(e->path,'.'));
    if(!callee) throw CodeGenError("Could not find function target "+join(e->path,'.'));

    for(int i = 0; i <params.size(); i++){
        auto v = BuildValue(params[i]);

        v = parent.CastTo(v, e->parameters[i]->type,path);
        args.push_back(v.value);
    }
    llvm::Value *returnValue = parent.builder.CreateCall(callee, args);

    return {returnValue, e->returnType};

}