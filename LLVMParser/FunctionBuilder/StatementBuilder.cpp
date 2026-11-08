#include "FunctionBuilder.hpp"
#include "llvm/IR/Instructions.h"
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
        locals.insert_or_assign(n->name, LocalVar{n->type, alloca});
    }
    if(auto n =TryCast<ReturnNode>(statement)){
        parent.builder.CreateRet(BuildValue(n->value).value);
    }
    if(auto n =TryCast<AssignNode>(statement)){
        auto target = GetAlloca(n->target);
        auto value = BuildValue(n->value);

        value = parent.CastTo(value, target.type, path);

        if(n->op != '='){
            value = BuildOprand(target, value, {n->op});
        }
        parent.builder.CreateStore(value.value, target.value);
    }
    if(auto n =TryCast<FuncCallNode>(statement)){

    }
    if(auto n =TryCast<IfNode>(statement)){

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
        if(locals.contains(n->name)){
            auto v = locals[n->name];
            return {v.AllocaInst,v.type};
        
        if(parent.globals.contains(n->name)){
            auto v = parent.globals[n->name];

            return {v.global,v.type};
        }
    }

    throw CodeGenError("Can not get allocator for this type of node");

    if(auto n = TryCast<FuncCallNode>(node)){
        throw CodeGenError("GetAlloca ExprNode-variation FuncCallNode uncounted for");
    }
    if(auto n = TryCast<StaticMemberNode>(node)){
        throw CodeGenError("GetAlloca ExprNode-variation StaticMemberNode uncounted for");
    }
    if(auto n = TryCast<InstanceMemberNode>(node)){
        throw CodeGenError("GetAlloca ExprNode-variation InstanceMemberNode uncounted for");
    }

}
