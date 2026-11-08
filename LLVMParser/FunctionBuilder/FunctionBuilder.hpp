#include "../LLVMBuilder.hpp"
#include "../Utillity/LLVMDebug.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include <map>

struct LLVMBuilder::FunctionBuilder
{ 
    struct LocalVar{
        Vartype type;
        llvm::AllocaInst* AllocaInst;
    };

    const FunctionNode* functionNode;
    Path path;

    // llvm parser
    LLVMBuilder& parent;

    // function builder 
    llvm::Function *fn;
    llvm::BasicBlock *entry;

    std::map<std::string, LocalVar> locals;
    FunctionBuilder(const FunctionNode* function, const Path& path, LLVMBuilder& parser)
        : functionNode(function), path(path), parent(parser){}

    llvm::Type* GetLLVMType(Vartype t){return parent.GetLLVMType(t,path);}
    llvm::Type* GetLLVMType(PrimitiveType t) {return parent.GetLLVMType(t);}

    // FunctionBuilder
    void BuildFunction();
    const LocalVar* GetLocalVar(std::string target);
    TypedValue GetAlloca(const ExprNode* node);
    
    // StatementBuilder
    void BuildStatement(const StatementNode* statement);

    // ValueBuilder
    TypedValue BuildValue(const ExprNode* node);
    TypedValue BuildOprand(TypedValue a, TypedValue b, std::string op){
        return parent.BuildOprand(a,b,op,path);
    }
};