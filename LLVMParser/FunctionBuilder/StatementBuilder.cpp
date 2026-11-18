#include "FunctionBuilder.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <vector>

#include "../../ASTParser/Debug/ASTPrinter.hpp"
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
        else if(n->type.vectorType){
            auto dataPtr = parent.GetStructField({alloca,n->type}, 0, path);
            auto sizePtr = parent.GetStructField({alloca,n->type}, 1, path);
            auto capPtr = parent.GetStructField({alloca,n->type}, 2, path);
            
            llvm::Value* capVal = parent.builder.getInt64(8);
            llvm::Value *mallocPtr = CreateMalloc({capVal, PrimitiveType::ULONG});
            parent.builder.CreateStore(parent.builder.getInt64(0), sizePtr.value);
            parent.builder.CreateStore(capVal, capPtr.value);
            parent.builder.CreateStore(mallocPtr, dataPtr.value);
            SetLocal(n->name, LocalVar{n->type, alloca});
            return;
        }else{
            init = llvm::Constant::getNullValue(ty);            
        }
        parent.builder.CreateStore(init, alloca); 
        SetLocal(n->name, LocalVar{n->type, alloca});
        return;
    }
    if(auto n =TryCast<ReturnNode>(statement)){
        TypedValue v = BuildValue(n->value);
        v = parent.CastTo(v, functionNode->returnType, path);
        parent.builder.CreateRet(v.value);
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
        return;
    }
    if(auto n =TryCast<IfNode>(statement)){
        PushScope();
        // true
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(parent.ctx, "if.then", fn);
        llvm::BasicBlock *elseBB = n->elseBody ? llvm::BasicBlock::Create(parent.ctx, "if.else", fn) : nullptr;
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(parent.ctx, "if.merge", fn);
        mergeBBs.push_back(mergeBB);
        TypedValue condition = BuildValue(n->condition);
            if(!condition.type.IsPrimitive() || condition.type.primitivetype.value() != PrimitiveType::BOOL)
                throw CodeGenError("Condition is not type bool");

        parent.builder.CreateCondBr(condition.value, thenBB, elseBB ? elseBB : mergeBB);
        parent.builder.SetInsertPoint(thenBB);
        for(auto statement : n->body->statements){
            BuildStatement(statement);
        }
        if (!parent.builder.GetInsertBlock()->hasTerminator()) 
            parent.builder.CreateBr(mergeBB);

        if (elseBB) {
            parent.builder.SetInsertPoint(elseBB);  
            for(auto statement : n->elseBody->statements){
                BuildStatement(statement);
            }
            if (!parent.builder.GetInsertBlock()->hasTerminator()) parent.builder.CreateBr(mergeBB);
        }
        parent.builder.SetInsertPoint(mergeBB);
        mergeBBs.pop_back();
        PopScope();
        return;
    }
    if(auto n =TryCast<WhileNode>(statement)){
        PushScope();
        BuildLoop(n,nullptr);
        PopScope();
        return;
    }
    if(auto n =TryCast<ForNode>(statement)){
        PushScope();
        BuildStatement(n->iteratordef);
        BuildLoop(n->loop, n->assignement);
        PopScope();
        return;
    }
    if(auto n =TryCast<ContinueNode>(statement)){
        if(thenBBs.empty())
            throw CodeGenError("Can not skip to next iteration: thenBB is null");
        parent.builder.CreateBr(thenBBs.back());
        return;
    }
    if(auto n =TryCast<BreakNode>(statement)){
         if(mergeBBs.empty())
            throw CodeGenError("Can not skip to next iteration: thenBB is null");
        parent.builder.CreateBr(mergeBBs.back());
        return;
    }
    throw CodeGenError("StatementNode variation Not implimented");
}

void LLVMBuilder::FunctionBuilder::BuildLoop(const WhileNode* node, const AssignNode* iterator){
    llvm::BasicBlock *condBB  = llvm::BasicBlock::Create(parent.ctx, "while.cond", fn);
    llvm::BasicBlock *bodyBB  = llvm::BasicBlock::Create(parent.ctx, "while.body", fn);
    llvm::BasicBlock *endBB = llvm::BasicBlock::Create(parent.ctx, "while.end", fn);

    thenBBs.push_back(bodyBB);
    mergeBBs.push_back(endBB);
    parent.builder.CreateBr(condBB);
    parent.builder.SetInsertPoint(condBB);
    TypedValue condition = BuildValue(node->condition);
    if(!condition.type.IsPrimitive() || condition.type.primitivetype.value() != PrimitiveType::BOOL)
        throw CodeGenError("Condition is not type bool");

    parent.builder.CreateCondBr(condition.value, bodyBB,  endBB);
    parent.builder.SetInsertPoint(bodyBB);

    for(auto statement : node->body->statements){
        BuildStatement(statement);
    }
    if(iterator)
        BuildStatement(iterator);
    if (!parent.builder.GetInsertBlock()->hasTerminator())
        parent.builder.CreateBr(condBB);

    parent.builder.SetInsertPoint(endBB);
    thenBBs.pop_back();
    mergeBBs.pop_back();
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
    if(auto n = TryCast<StringNode>(node)){
        
        return {parent.builder.CreateGlobalString(n->value),Vartype::NewPtr(PrimitiveType::CHAR)};
    }
   
    
    if(auto n = TryCast<FuncCallNode>(node)){
        TypedValue v = CreateFuncCall(n);
        auto alloca = parent.builder.CreateAlloca(GetLLVMType(v.type),nullptr, "tempAlloca");
        parent.builder.CreateStore(v.value, alloca);
        return{alloca,v.type};
    }

    std::ostringstream s;
    ASTPrinter::printExpr(s, node, {});
    throw CodeGenError("Can not get allocator for this type of node:"+s.str());
    if(auto n = TryCast<InstanceMemberNode>(node)){
        throw CodeGenError("GetAlloca ExprNode-variation InstanceMemberNode uncounted for");
    }

}


void LLVMBuilder::FunctionBuilder::CreatePrintf(const FuncCallNode* fn){
    if (fn->params->params.empty())
        throw CodeGenError("print takes at least 1 argument");
    for(auto* n : fn->params->params){
        auto alloca = GetAlloca(n);
        CreatePrintf(alloca);
    }
}

void LLVMBuilder::FunctionBuilder::CreatePrintf(TypedValue alloca){
    llvm::Value* str_c;
    if(alloca.type.IsString()){
        auto str_ptr = parent.GetStructField(alloca, 0, path);
        str_c = parent.builder.CreateLoad(GetLLVMType(str_ptr.type),  str_ptr.value);
    }else if(alloca.type.IsPointer() && alloca.type.pointee->primitivetype == PrimitiveType::CHAR){
        str_c = alloca.value;
    }
    else{
        alloca = CreateSnprintfFn(alloca);
        str_c = alloca.value;
    }
    parent.builder.CreateCall(printfFn, {str_c});
}

LLVMBuilder::TypedValue LLVMBuilder::FunctionBuilder::CreateFuncCall(const FuncCallNode* n){
    if(auto f = TryCast<InstanceFuncCallNode>(n)){
        return CreateInstanceFuncCall(f);
    }

    if(n->funcname == "print"){ 
        CreatePrintf(n);
        return {nullptr, PrimitiveType::VOID};
    }
    auto params = n->params->params;
    std::vector<llvm::Value*> args;
    auto e = parent.symbolScanner.FindFunction({n->funcname}, path);
    if(!e) throw CodeGenError("Could not find function: "+n->funcname);
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

LLVMBuilder::TypedValue LLVMBuilder::FunctionBuilder::CreateInstanceFuncCall(const InstanceFuncCallNode* n){
    TypedValue alloca = GetAlloca(n->object);
    if(alloca.type.vectorType){
        if(n->funcname == "push_back"){
            if(n->params->params.size() != 1) throw CodeGenError("vector.push_back takes only one argument");
            CreatePushBack(alloca,BuildValue(n->params->params[0]));
            return {nullptr, PrimitiveType::VOID};
        }
        if(n->funcname == "size"){
            if(n->params->params.size() != 0) throw CodeGenError("vector.size() takes 0 arguments");
            auto sizeF = parent.GetStructField(alloca, 1, path);
            llvm::Value *sizeVal = parent.builder.CreateLoad(GetLLVMType(sizeF.type), sizeF.value, "sizeVal");
            
            if(alloca.type.vectorType->IsString()) parent.builder.CreateAnd(sizeVal,parent.builder.getInt64(~STR_CONST_FLAG));
            return {sizeVal,sizeF.type};
        }
    }
    throw CodeGenError("Invalid InstanceFuncCall: "+n->funcname);
}


LLVMBuilder::TypedValue LLVMBuilder::FunctionBuilder::CreateSnprintfFn(TypedValue alloca){
    if(!alloca.type.IsPrimitive()) throw CodeGenError("CreateSnprintfFn only print primitive");
    auto& b = parent.builder;
    llvm::Value* buffer = b.CreateAlloca(b.getInt8Ty(), b.getInt32(32), "snprint_buffer");

    TypedValue v = {b.CreateLoad(GetLLVMType(alloca.type),alloca.value,"snprint_value"),alloca.type};
    std::string format;
    switch(alloca.type.primitivetype.value()){
        case PrimitiveType::UCHAR:
        case PrimitiveType::USHORT:
        case PrimitiveType::UINT:
            format = "%u"; v = parent.CastTo(v, PrimitiveType::UINT, path); break;
        case PrimitiveType::CHAR:
        case PrimitiveType::BOOL:
        case PrimitiveType::SHORT:
        case PrimitiveType::INT:
            format = "%d"; v = parent.CastTo(v, PrimitiveType::INT, path); break;
        case PrimitiveType::LONG:
            format = "%lld"; break;
        case PrimitiveType::ULONG:
            format = "%llu"; break;
        case PrimitiveType::FLOAT:
        case PrimitiveType::DOUBLE:
            format = "%g"; v = parent.CastTo(v, PrimitiveType::DOUBLE, path); break;
        case PrimitiveType::VOID:
            throw CodeGenError("Invalid type VOID in CreateSnprintfFn");
    }
    
    llvm::Value* fmtStr = b.CreateGlobalString(format,"snprint_format"); 

    b.CreateCall(snprintfFn,{
        buffer,
        b.getInt64(32),
        fmtStr,
        v.value
    });

    return {buffer, Vartype::NewPtr(PrimitiveType::CHAR)};
}

llvm::Value* LLVMBuilder::FunctionBuilder::CreateMalloc(TypedValue size){
    if(!size.type.IsPrimitive(PrimitiveType::ULONG)) throw CodeGenError("Can not CreateMalloc with size non ULONG");
    auto& b = parent.builder;

    return b.CreateCall(mallocFn, {size.value});
}

void LLVMBuilder::FunctionBuilder::CreateFree(TypedValue dataPtr){
    if(!dataPtr.type.IsPointer()) throw CodeGenError("Can not Free with size non Pointer");
    auto& b = parent.builder;
    b.CreateCall(freeFn, {dataPtr.value});
}