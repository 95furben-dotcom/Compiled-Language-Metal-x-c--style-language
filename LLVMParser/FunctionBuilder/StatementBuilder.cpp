#include "FunctionBuilder.hpp"
void LLVMBuilder::FunctionBuilder::BuildStatement(const StatementNode* statement){
    if(auto n =TryCast<VarDefNode>(statement)){
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
    if(auto n =TryCast<FuncCallNode>(statement)){

    }
    if(auto n =TryCast<AssignNode>(statement)){

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
}

