#pragma once
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include <cstddef>
#include <map>

#include "../ASTParser/AST.hpp"
#include "Symbolscanner/SymbolScanner.hpp"

struct LLVMBuilder{
    struct TypedValue {
        llvm::Value* value;
        Vartype type;      
        TypedValue(llvm::Value* value,
        Vartype type) : value(value), type(type){}
    };

    struct GlobalVar {
        llvm::GlobalVariable* global;
        llvm::Type* llvmType;
        Vartype type;            
    };

    llvm::LLVMContext ctx;
    llvm::Module mod = llvm::Module("program", ctx);
    llvm::IRBuilder<> builder = llvm::IRBuilder<>(ctx);

    SymbolScanner symbolScanner;

    
    std::map<std::string, GlobalVar> globals;   // key: join(path, '.')

    LLVMBuilder(const NamespaceNode* program)
        : symbolScanner(program) {}

    static void ASTToLLVM(const NamespaceNode* program){
        LLVMBuilder parser(program);
        parser.BuildStaticVariables();
        parser.DeclareFunctions();
        parser.BuildNamespace(program);
    }

    // LLVMBuilder
    void BuildNamespace(const NamespaceNode* namespaceNode, std::vector<std::string> path = {});
    void BuildStaticVariables();
    void BuildStruct(const StructNode* structNode, std::vector<std::string> path = {});
    void DeclareFunctions();

    GlobalVar* FindGloabal(const Path& targetPath);
    // GetType
    llvm::Type* GetLLVMType(const Vartype& t, const Path& path);
    llvm::Type* GetLLVMType(PrimitiveType t);

    // ValueBuilder
    TypedValue BuildValue(const ExprNode* node, const Path& path);
    TypedValue BuildOprand(TypedValue a, TypedValue b, const std::string& op, const Path& path);
    TypedValue CastTo(TypedValue v, const Vartype& target, const Path& path);
    llvm::Constant* BuildConstant(const ExprNode* node, const Vartype& target, const Path& path);
    bool isSigned(const Vartype& a);

    
    struct FunctionBuilder;

    void WriteToFile(const std::string& filepath);
};

