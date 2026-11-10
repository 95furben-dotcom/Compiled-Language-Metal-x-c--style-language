#include "../LLVMBuilder.hpp"
#include "../Utillity/LLVMDebug.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include <map>
#include <string>
#include <vector>

struct LLVMBuilder::FunctionBuilder
{ 
    struct LocalVar{
      LocalVar(Vartype type, llvm::AllocaInst *AllocaInst)
          : type(std::move(type)), AllocaInst(AllocaInst) {}
      Vartype type;
      llvm::AllocaInst *AllocaInst;
    };

    const FunctionNode* functionNode;
    Path path;

    // llvm parser
    LLVMBuilder& parent;

    // function builder 
    llvm::Function *fn;
    llvm::BasicBlock *entry;

    std::vector<std::map<std::string, LocalVar>> scopes;

    std::vector<llvm::BasicBlock*> thenBBs;
    std::vector<llvm::BasicBlock*> mergeBBs;

    void PushScope() {scopes.emplace_back();}
    void PopScope() {scopes.pop_back();}
    bool HasLocal(std::string key);

    void SetLocal(std::string key, LocalVar var) {scopes.back().insert_or_assign(key, var);}

    FunctionBuilder(const FunctionNode* function, const Path& path, LLVMBuilder& parser)
        : functionNode(function), path(path), parent(parser), scopes(){
        PushScope();
    }

    llvm::Type* GetLLVMType(Vartype t){return parent.GetLLVMType(t,path);}
    llvm::Type* GetLLVMType(PrimitiveType t) {return parent.GetLLVMType(t);}

    // FunctionBuilder
    void BuildFunction();
    const LocalVar* GetLocalVar(std::string target);
    TypedValue GetAlloca(const ExprNode* node);
    
    // StatementBuilder
    void BuildStatement(const StatementNode* statement);
    TypedValue CreateFuncCall(const FuncCallNode* n);
    
    //DO PUSHSCOPE BEFORE RUNNING
    void BuildLoop(const WhileNode* node, const AssignNode* iterator);

    // ValueBuilder
    TypedValue BuildValue(const ExprNode* node);
    TypedValue BuildOprand(TypedValue a, TypedValue b, std::string op){
        return parent.BuildOprand(a,b,op,path);
    }
};