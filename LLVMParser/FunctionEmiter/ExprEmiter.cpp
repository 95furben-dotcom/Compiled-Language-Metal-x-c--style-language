#include "FunctionEmiter.hpp"
#include "../Utility/Oprands.hpp"
#include <string>

#include "../Utillity/Caster.hpp"
#include "../CodeGenError.hpp"


Vartype FunctionEmiter::emitExpr(const ExprNode* exprNode, uint resultStackPos){
    CALLSTACKADD();
    if(exprNode == nullptr) throw CodeGenError("Can not emit nullptr exprnode");
    // emit("\n");
    // emitCommentNl("Emiting exprnode at resultStackPos: "+std::to_string(resultStackPos));
    // emitNodeComment(exprNode);
    // result end up on the top of the stack
    // will use the stack, but delete when done
    
    // NumberNode OprandNode VariableCallNode FuncCallNode StaticMemberNode StaticFuncCallNode InstanceFuncCallNode InstanceMemberNode     
    if(ExprHasDestination(exprNode)){
        Dest src = FindExprDestination(exprNode);
        Vartype type = src.entry->vatiableInstance.type;

        Dest dest = Dest::StackDest(resultStackPos, nullptr);
        emitMemCopy(src, dest, symbolScanner.GetVartypeSize(type, path));
        return src.entry->vatiableInstance.type;
    }
    if(auto n  = TryCast< NumberNode>(exprNode)){
        if(n->type == Defaulttype::STRING){
            // get pointer, 
            emitString(n->value,resultStackPos);
            return Vartype(n->type);
        }
        emitMov(n->value, n->type , 0);
        emitStoreFromReg(GetReg(0, n->type),resultStackPos);
        return Vartype(n->type);
    }
    if(auto n  = TryCast< OprandNode>(exprNode)){
        return emitOprand(n->v1, n->v2, n->oprand, resultStackPos);
    }
    if(auto n  = TryCast< FuncCallNode>(exprNode)){
        Vartype t = emitFunccall(n, resultStackPos);
        if(t.defaulttype == Defaulttype::VOID)
            throw CodeGenError("Can use void funccall as expr");
        return t;
    }
    throw CodeGenError("ExprNode unaccounted for");
}

Vartype FunctionEmiter::LoadExprToReg(const ExprNode* node, uint reg, uint exprStackTop){
    if(ExprHasDestination(node)){
        Dest dest = FindExprDestination(node);
        dest.emitLoadDestAddr("x9");
        dest.destOperand(0);
        if(!dest.entry) 
            throw CodeGenError("Dest does not have entry");
        return dest.entry->vatiableInstance.type;
    }
    // has no destination: emitExpr
    Vartype type = emitExpr(node, exprStackTop);
    emitLoadToReg("x"+std::to_string(reg), exprStackTop);
    return type;
}


Vartype FunctionEmiter::emitOprand(const ExprNode* a, const ExprNode* b, std::string op, uint resultStackPos){
    CALLSTACKADD();
    // restult at top 
    // if expr a and b has a destination, load to reg from there 
    
    Vartype typeA = LoadExprToReg(a,0,resultStackPos);
    
    // result at top + size(typeA)
    uint stackB = resultStackPos + (ExprHasDestination(a) ? getSize(typeA) : 0);
    Vartype typeB = LoadExprToReg(b,1,stackB);

    if(typeA.defaulttype == Defaulttype::STRUCT || typeB.defaulttype == Defaulttype::STRUCT)
        throw CodeGenError("Oprands between structs not implimented");
    
    emitCommentNl("Do the operation: "+op);
    // result at regA
    Vartype returntype = Oprands::EmitOprand(typeA.defaulttype, typeB.defaulttype, op);
    std::string resultReg = GetReg(0, returntype.defaulttype);


    // load to stack
    emitCommentNl("load result to stack from reg "+resultReg);
    emitInstruction("str "+resultReg+", [sp, #"+std::to_string(resultStackPos)+"]");

    return returntype;
}


Vartype FunctionEmiter::emitFunccall(const FuncCallNode* funccallnode, uint resultStackPosition){
    if(funccallnode == nullptr) throw CodeGenError("funccallnode is null");
    /*
    if the returntype is not defaultype, the fist parameter is a long, pointing to where it wants the output on stack
    the callie first
    puts variables on stack in [sp, #N]
    then calls func2
    func 2 can acces the variables as
    ldr w0, [x0]

    now, the result from the called function is either in w0, x0, f0 or d0 
    or if it is not defaulttype, in the pointer 
    */
    CALLSTACKADD();
    SymbolEntry entry = GetFuncCallEntry(funccallnode);

    // assign pointer to resultStackPosition
    emitInstruction("add x0, sp, #"+std::to_string(resultStackPosition));

    if(auto n = TryCast< InstanceFuncCallNode>(funccallnode)){
        if(n->object == nullptr)throw CodeGenError("InstanceFuncCallNode has nullptr for object");

        Vartype type = emitExpr(n->object,  8);
        Path parentPath = FindExprDestination(n->object).entry->vatiableInstance.type.structPath;

        if(type.defaulttype != Defaulttype::STRUCT || type.structPath != parentPath) 
            throw CodeGenError("Invalid funccall: can't assign variables to incorrect type");
        
        // assign pointers to 
        emitLoadToReg("x1", 8);

        for(uint i =  0; i < n->params.size(); i++){
            if(funccallnode->params[i] == nullptr)throw CodeGenError("Param nr. "+std::to_string(i)+" is nullptr");
            Vartype type = emitExpr(funccallnode->params[i],  i*8);
            if(entry.parameters[i].defaulttype != type.defaulttype
                || (type.defaulttype == Defaulttype::STRUCT && type.structPath != entry.parameters[i].structPath)) 
                throw CodeGenError("Invalid funccall: can't assign variables to incorrect type");
            
            // assign pointers to 
            emitLoadToReg("x"+std::to_string(i + 2), i*8 + 16);
        }
    }
    else{
        for(uint i =  0; i < funccallnode->params.size(); i++){
            if(funccallnode->params[i] == nullptr)throw CodeGenError("Param nr. "+std::to_string(i)+" is nullptr");
            Vartype type = emitExpr(funccallnode->params[i],  i*8);
            if(entry.parameters[i].defaulttype != type.defaulttype
                || (type.defaulttype == Defaulttype::STRUCT && type.structPath != entry.parameters[i].structPath)) 
                throw CodeGenError("Invalid funccall: can't assign variables to incorrect type");
            
            // assign pointers to 
            emitLoadToReg("x"+std::to_string(i+1), i*8 + 8);
        }
    }

    
    
    emitInstruction("bl _" + GetParsedVariableName(entry.path));


    return entry.vatiableInstance.type;
}


void FunctionEmiter::emitString(const std::string& s, uint resultStackPosition){
    // floats can't be mov'd directly, must go through a data label
    uint idx = dataManager.addString(s);
    std::string label = "_s" + std::to_string(idx);
    emitInstruction("adrp x0, " + label + "@PAGE");
    emitInstruction("add  x0, x0, " + label + "@PAGEOFF");
    // pointer is now in x0

    emitMov(std::to_string(s.size()),Defaulttype::ULONG,1);
    // size in now in x1

    emitInstruction("mov x2, #0");
    // capacity in now in x2

    emitStoreFromReg("x0", resultStackPosition);
    emitStoreFromReg("x1", resultStackPosition + 8);

    // use first bit for knowing if pointer is to heap or const 
    // 1 is mutatable : our string 
    emitStoreFromReg("x2", resultStackPosition + 16);
}