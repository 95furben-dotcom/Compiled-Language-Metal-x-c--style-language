#include "FunctionEmiter.hpp"
#include "../Utility/Emiting/NodeToComment.hpp"


inline uint FunctionEmiter::getSize(Vartype t){
    return symbolScanner.GetVartypeSize(t, path);
}

void FunctionEmiter::seedLocalTypes(const FunctionNode* node){
    for(auto* p : node->params)
        values[p->name] = VariableInstance(p->isConst, p->isRefrance, p->type, 0, 0); // offset/size unused here
    for(auto* s : node->body)
        if(auto* v = TryCast< VarDefNode>(s))
            values[v->name] = VariableInstance(v->isConst, v->isRefrance, v->type, 0, 0);
}

const SymbolEntry& FunctionEmiter::GetFuncCallEntry(const FuncCallNode* node){
    CALLSTACKADD();
    if(auto n = TryCast< StaticFuncCallNode>(node)){
        auto e = symbolScanner.functionInfos.find(n->path, path);
        if(!e) throw CodeGenError("Could not find StaticFuncCallNode path:"+join(n->path,'.'));
        if(!e->isFunctionStatic)throw CodeGenError("Found function "+join(e->path,'.')+" but StaticFuncCallNode must be static");
        return *e;
    }
    if(auto n = TryCast< InstanceFuncCallNode>(node)){
        Vartype parentType = getExprVarType(n->object);
        if(parentType.defaulttype != Defaulttype::STRUCT)
            throw CodeGenError("Can not have InstanceFuncCallNode on non-struct types");
        if(parentType.defaulttype == Defaulttype::VOID) throw CodeGenError("Cant have InstanceFuncCallNode on void type");
        const SymbolEntry *parentEntry = symbolScanner.structInfos.find(parentType.structPath, path);
        if(!parentEntry) throw CodeGenError("Could not find parent entry");
        Path funcPath = parentEntry->path;
        funcPath.push_back(n->funcname);
        auto e = symbolScanner.functionInfos.find(funcPath, path);
        if(!e) throw CodeGenError("Could not find InstanceFuncCallNode path:"+join(funcPath,'.'));
        if(e->isFunctionStatic)throw CodeGenError("Found fucntion "+join(e->path,'.')+" but InstanceFuncCallNode cant be static");
        return *e;
    }
    auto e = symbolScanner.functionInfos.find({node->funcname}, path);
    if(!e) throw CodeGenError("Could not find FuncCallNode: "+node->funcname);
    if(!e->isFunctionStatic)throw CodeGenError( "Found function "+join(e->path,'.')+" but FuncCallNode must be static");
    return *e;    
}

uint FunctionEmiter::calcExprTempSize(const ExprNode* n) {
    CALLSTACKADD();
    if (!n) return 0;
    
    if (auto a = TryCast< NumberNode>(n))
        return getPrimitiveSize(a->type);

    if (auto op = TryCast< OprandNode>(n)) {
        // need space for both sides simultaneously
        uint sizeA = calcExprTempSize(op->v1);
        uint sizeB = calcExprTempSize(op->v2);
        return sizeA + sizeB;
    }

    if (TryCast< VariableCallNode>(n))
        return 0; // already on stack, no temp needed

    if (TryCast< StaticMemberNode>(n))
        return 0; // same, already exists

    if (TryCast< InstanceMemberNode>(n))
        return 0; // same

    if (auto* fc = TryCast< FuncCallNode>(n)) {
        uint size;
       
        if (auto* ifc = TryCast< InstanceFuncCallNode>(n)) {
            size += calcExprTempSize(ifc->object);
        }
        const SymbolEntry &entry = GetFuncCallEntry(fc);
        size = getSize(entry.vatiableInstance.type); // return slot
        

        for (auto* p : fc->params)
            size += calcExprTempSize(p);
        return size;
    }

    throw CodeGenError("calcExprTempSize: unhandled ExprNode");
}

uint FunctionEmiter::calcMaxExprTempSize(const FunctionNode* node){
    CALLSTACKADD();
    uint maxSize = 0;
    for(auto* s : node->body){
        if(auto n = TryCast< VarDefNode>(s)){
            maxSize = std::max(maxSize, calcExprTempSize(n->value));
        }
        if(auto n = TryCast< ReturnNode>(s)){
            maxSize = std::max(maxSize, calcExprTempSize(n->value));
        }
        if(auto n = TryCast< AssignNode>(s)){
            maxSize = std::max(maxSize, calcExprTempSize(n->value));
        }
    }
    return maxSize;
}


uint FunctionEmiter::calcStackSize(const FunctionNode* node){
    CALLSTACKADD();
    seedLocalTypes(node);
    uint size = 16; // x29 and x30
    // params
    for(const auto* p : node->params)
        size += getSize(p->type);
    // local vars in body
    for(const auto* s : node->body){
        if(auto* v = TryCast< VarDefNode>(s))
            size += getSize(v->type);
    }

    // add temp space for expression evaluation
    size += calcMaxExprTempSize(node);
    // round up to 16-byte alignment, +16 for x29/x30

    values.clear();
    return ((size + 16 + 15) & ~15);
}

void FunctionEmiter::PushToStack(const VarDefNode* node){
    CALLSTACKADD();
    uint size = getSize(node->type);
    VariableInstance v(node->isConst, node->isRefrance, node->type, stackTop, symbolScanner.GetVartypeSize(node->type,path));
    stackTop += size;
    values[node->name] = v;
}

const SymbolEntry& FunctionEmiter::GetExprSymbolEntry(const ExprNode* expn, const Path& scope){
    CALLSTACKADD();
    /*
        FuncCallNode
        ├── StaticFuncCallNode 
        └── InstanceFuncCallNode
        StaticMemberNode
        InstanceMemberNode
    */
    if(auto* n = TryCast< StaticMemberNode>(expn)){
        auto e = symbolScanner.staticVariables.find(n->path, scope);
        if(!e) throw std::runtime_error("Could not find symbolentry for StaticMemberNode: "+join(n->path,'.'));
        return *e;
    }

    if(auto* n = TryCast< InstanceMemberNode>(expn)){
        // parent is varcallnode or InstanceMemberNode; find type of parent, then use to find field in struct

        // parent type
        Vartype parentType = getExprVarType(n->object);
        if(parentType.defaulttype != Defaulttype::STRUCT)
            throw CodeGenError("Cant find fields in defaultypes");

        const SymbolEntry* entry = symbolScanner.structInfos.find(parentType.structPath, scope);
        if(!entry) throw std::runtime_error("Could not find symbolentry for InstanceMemberNode: "+join(parentType.structPath,'.'));
        auto e = entry->findField(n->field);
        if(!e) throw std::runtime_error("Could not find symbolentry for field at "+join(parentType.structPath,'.')+ "field: "+n->field);
        return *e;
    }
    if(auto* n = TryCast< FuncCallNode>(expn))
        return GetFuncCallEntry(n);

    throw CodeGenError("Exprnode variation uncounted for");

}

Vartype FunctionEmiter::getExprVarType(
    const ExprNode* expn
){
    CALLSTACKADD();
    if(!expn)
        throw CodeGenError("Cant get type of null");
    /*
        NumberNode
        OprandNode
        VariableCallNode
        FuncCallNode
        ├── StaticFuncCallNode
        └── InstanceFuncCallNode
        StaticMemberNode
        InstanceMemberNode
    */
    if(auto* n = TryCast< NumberNode>(expn)){
        return Vartype(n->type);
    }
    if(auto* n = TryCast< OprandNode>(expn)){
        return getExprVarType(n->v1);
    }
    if(auto* n = TryCast< VariableCallNode>(expn)){
        if(values.contains(n->name))
            return values[n->name].type;
        if(!isStatic){
            if(objectEntry == nullptr)  throw CodeGenError("WHY IS objectEntry nullptr when is not statc???");
            auto f = objectEntry->findField(n->name);
            if(f) return f->vatiableInstance.type;
        }

        auto e = symbolScanner.staticVariables.find({n->name}, path);
        if(!e) throw CodeGenError("Did not find entry for VariableCallNode: "+n->name+" path: "+join(path, '.'));
        return e->vatiableInstance.type;
    }
    
    /*
        FuncCallNode
        ├── StaticFuncCallNode
        └── InstanceFuncCallNode
        StaticMemberNode
        InstanceMemberNode
    */
    return GetExprSymbolEntry(expn ,path).vatiableInstance.type;
}

void FunctionEmiter::emitProloge(uint stackSpaceNeeded){
    CALLSTACKADD();
    // reserve space on stack
    emitCommentNl("reserve space on stack");
    emitInstruction("sub sp, sp, #"+std::to_string(stackSpaceNeeded));
    // save frame pointer and link register
    emitInstruction("stp x29, x30, [sp, #"+std::to_string(stackSpaceNeeded - 16)+"]");
    emitInstruction("mov x29, sp"); // store bookmark to stack

    emit("");
}

void FunctionEmiter::emitNodeComment(const Node* n){
    emit("\t// "+NodeToComment::NodeToComment(n));
}