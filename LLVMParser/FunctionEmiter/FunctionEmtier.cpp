#include "FunctionEmiter.hpp"
#include <cstddef>
#include <string>
void FunctionEmiter::emitFunction(const FunctionNode* node){
    CALLSTACKADD();
    emitLabel("_"+GetParsedVariableName(path));
    emitProloge(stackSpaceNeeded);
    stackTop = 16;
    
    // parameters are all just refrances stored in x0 -> x30 giving 31 max params 
    // argument one is a pointer to where the return should be placed, load this long to stack

    emitCommentNl("Store output pointer to stack");
    emitStoreFromReg("x0", 0);

    emitCommentNl("Store parameters to stack from pointers");    

    for(uint i = 0; i < node->params.size(); i++){
        VarDefNode* p = node->params[i];
        PushToStack(p);
    }

    // emit statements 
    for(auto& s : node->body){
        emitStatement(s);
    }
    emit("");
}

bool FunctionEmiter::ExprHasDestination(const ExprNode* exprNode){
    return (TryCast< StaticMemberNode>(exprNode) || 
        TryCast< InstanceMemberNode>(exprNode) ||
        TryCast< VariableCallNode>(exprNode));
}

Dest FunctionEmiter::FindExprDestination(const ExprNode* exprNode){
    CALLSTACKADD();
    if(auto n  = TryCast< StaticMemberNode>(exprNode)){
      const SymbolEntry* entry = symbolScanner.staticVariables.find(n->path, path);
      if(!entry) throw CodeGenError("Count not find static entry");
      return Dest::StaticDest(n->path, entry);
    }
    if(auto n  = TryCast< InstanceMemberNode>(exprNode)){
        auto parentDest = FindExprDestination(n->object);
        const SymbolEntry *entry = parentDest.entry->findField(n->field);
        if(!entry) throw CodeGenError("Could not find entry for InstanceMemberNode");
        parentDest.label.push_back(n->field);

        parentDest.offset += entry->vatiableInstance.stackOffset;
        parentDest.entry = entry;
        return parentDest;
    }
    if(auto n  = TryCast< VariableCallNode>(exprNode)){
        if(values.contains(n->name)){
            VariableInstance instance = values[n->name];
            SymbolEntry* entry  = new SymbolEntry({}, n, instance);
            return Dest::StackDest(instance.stackOffset, entry);
        }
        if(!isStatic)
            if(auto a = objectEntry->findField(n->name)){
                return Dest::PointerDest(8, a->vatiableInstance.stackOffset, a);
            }
        const SymbolEntry* entry = symbolScanner.FindEntry({n->name}, path);
        if(!entry) throw CodeGenError("could not get entry for ");
        return Dest::StaticDest(entry->path, entry);
    }
    throw CodeGenError("Invalid exprnode in emitAllocateExpr");
}

void FunctionEmiter::emitAllocateExpr(const ExprNode* srcNode, const ExprNode* destNode){
    CALLSTACKADD();
    // allocate value
    Dest src = Dest::StackDest(stackTop, nullptr);
    emitExpr(srcNode,0);

    // destination is a static variable, or at stack 

    // find out destination 
    Dest dest = FindExprDestination(destNode);
    emitMemCopy(src, dest, symbolScanner.GetVartypeSize(getExprVarType(destNode), path));
}


void FunctionEmiter::emitStatement(const StatementNode* s){
    CALLSTACKADD();
    emit("");
    
    //emit statemient
    emitNodeComment(s);

    if(auto* n = TryCast< ReturnNode>(s)){
        // is return 
        if(returnType.defaulttype == Defaulttype::VOID){
            emitInstruction("ret");
            return;
        }
        if(!n->value) throw CodeGenError("Can return uninitalized value");
        uint resultStackIndex = stackTop;
        emitExpr(n->value,stackTop);
        Dest src = Dest::StackDest(resultStackIndex, nullptr);

        // we must allocate our output to the pointer 
        // retrive pointer to target space on stack
        // pointer to destination
        emitInstruction("add x0, sp, #"+std::to_string(resultStackIndex));
        Dest dest = Dest::PointerDest(0, 0, nullptr);

        emitMemCopy(src, dest, getSize(returnType));


        emitCommentNl("epilogue");
        emitInstruction("ldp x29, x30, [sp, #"+std::to_string(stackSpaceNeeded)+"-16]");
        
        emitCommentSl("restore frame pointer and link register");
        emitInstruction("add sp, sp, #"+std::to_string(stackSpaceNeeded));
        emitCommentSl("give back stack space");
        emitInstruction("ret");
        emitCommentSl("jump to x30");
    }

    if(auto* n = TryCast< VarDefNode>(s)){
        if(n->isStatic) throw CodeGenError("Can not have static variables in functions");
        if(n->value){
            emitExpr(n->value,stackTop); // result on top of stack
        }else{
            uint size = symbolScanner.GetVartypeSize(n->type, path);
            emitCommentNl("push defaultvalue to stack: "+ std::to_string(size));
            // push defaultvalue to stack: 0
            emitInstruction("mov x0, #0");
            for(uint i = 0; i < size / 8; i++){
                emitStoreFromReg("x0", stackTop + i*8);
            }
            size %= 8;
            for(uint i = 0; i < size / 4; i++){
                emitStoreFromReg("w0", stackTop + i*4);
            }
            size %= 4;
            for(uint i = 0; i < size; i++){
                emitInstruction("strb w0, [sp, #"+std::to_string(i)+"]");
            }
        }

        
        PushToStack(n);
    }


    if(auto* n = TryCast< AssignNode>(s)){
        if(n->op == '=')
            emitAllocateExpr(n->value, n->target);
        else{
            emitOprand(n->target, n->value, {n->op},0);

        }
    }
    if(auto* n = TryCast< FuncCallNode>(s)){
        // will be destroyed when crating new variables
        emitFunccall(n, stackTop); 
    }   


}

void FunctionEmiter::emitMov(std::string value, Defaulttype t, uint regIndex) {
    CALLSTACKADD();
    std::string reg;
    switch (t) {
        case Defaulttype::CHAR:
        case Defaulttype::BOOL:
        case Defaulttype::SHORT:
        case Defaulttype::USHORT:
        case Defaulttype::INT:
        case Defaulttype::UINT:
            reg = "w" + std::to_string(regIndex);
            emitInstruction("mov " + reg + ", #" + value);
            break;

        case Defaulttype::LONG:
        case Defaulttype::ULONG:
            reg = "x" + std::to_string(regIndex);
            emitInstruction("mov " + reg + ", #" + value);
            break;

        case Defaulttype::FLOAT: {
            // floats can't be mov'd directly, must go through a data label
            uint idx = dataManager.addFloat(std::stof(value));
            std::string label = "_f" + std::to_string(idx);
            emitInstruction("adrp x9, " + label + "@PAGE");
            emitInstruction("add  x9, x9, " + label + "@PAGEOFF");
            emitInstruction("ldr  s" + std::to_string(regIndex) + ", [x9]");
            break;
        }

        case Defaulttype::DOUBLE: {
            uint idx = dataManager.addDouble(std::stod(value));
            std::string label = "_d" + std::to_string(idx);
            emitInstruction("adrp x9, " + label + "@PAGE");
            emitInstruction("add  x9, x9, " + label + "@PAGEOFF");
            emitInstruction("ldr  d" + std::to_string(regIndex) + ", [x9]");
            break;
        }

        default:
            throw CodeGenError("Unhandled type in emitMov"+ std::to_string((uint)t));
    }
}


