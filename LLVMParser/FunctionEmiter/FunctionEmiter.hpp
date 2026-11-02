#pragma once
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <vector>
#include "../SymbolScanner/SymbolScanner.hpp"
#include "../LLVMParser.hpp"
#include "../CodeGenError.hpp"
/*
Model:
all functions are static, but the ones that are not are always in a struct
in this case, it gets the first argument as a refrance to space on stack
*/
struct FunctionEmiter{
    const Path& path;
    const Symbolscanner& symbolScanner;
    LLVMParser& llvmparser;

    std::unordered_map<std::string, int> values;

    Vartype returnType;
    
    // if this is nullptr, then it is static
    // pointer to the parent is in [sp, #8]
    const FunctionEntry* objectEntry = nullptr;
    bool isStatic;

    uint stackSpaceNeeded;
    
    FunctionEmiter(const Symbolscanner& symbolScanner, const Path& path, const FunctionEntry* objectEntry, const FunctionNode* functionNode, LLVMParser& llvmparser ) 
        : path(path), symbolScanner(symbolScanner), values(), llvmparser(llvmparser),
        returnType(functionNode->returnType),stackSpaceNeeded(calcStackSize(functionNode)),objectEntry(objectEntry), isStatic(objectEntry == nullptr){
            CALLSTACKADD();
            if(isStatic != functionNode->specifiers)  
                throw CodeGenError("Non-static function has objectEntry = nullptr ");

    }

    void emitFunction(const FunctionNode* node);
    private:

    uint getSize(Vartype t);

    void seedLocalTypes(const FunctionNode* node);

    uint calcExprTempSize(const ExprNode* n);
    uint calcMaxExprTempSize(const FunctionNode* node);
    uint calcStackSize(const FunctionNode* node);

    void PushToStack(const VarDefNode* node);

    const VariableEntry& GetExprSymbolEntry(const ExprNode* expn, const Path& scope);

    Vartype getExprVarType(const ExprNode* expn);
  

    const FunctionEntry& GetFuncCallEntry(const FuncCallNode* node);

    static void emitProloge(uint stackSpaceNeeded);

    void emitStatement(const StatementNode* s);

    Vartype emitFunccall(const FuncCallNode* funccallnode, uint resultStackPosition);
    Vartype emitExpr(const ExprNode* n, uint resultStackPos);
    Vartype emitOprand(const ExprNode* a, const ExprNode* b, std::string op, uint resultStackPos);
    Vartype LoadExprToReg(const ExprNode* node, uint reg, uint exprStackTop);
    void emitString(const std::string& s, uint resultStackPosition);

    void emitMov(std::string value, PrimitiveType t, uint regindex);
    uint GetVarCallNodeStackPos(const ExprNode* e);
    void emitAllocateExpr(const ExprNode* srcNode, const ExprNode* destNode);

    bool ExprHasDestination(const ExprNode* exprNode);
    Dest FindExprDestination(const ExprNode* exprNode);
    void emitNodeComment(const Node* n);
};
