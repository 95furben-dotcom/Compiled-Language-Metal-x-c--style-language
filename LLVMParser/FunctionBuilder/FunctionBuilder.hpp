#include "../LLVMParser.hpp"
#include "../Utillity/LLVMDebug.hpp"
#include <map>

struct LLVMParser::FunctionBuilder
{ 
    const FunctionNode* functionNode;
    Path path;

    // llvm parser
    llvm::LLVMContext& ctx;
    llvm::Module& mod;
    llvm::IRBuilder<>& builder;
    const SymbolScanner& symbolScanner;

    std::map<std::string, llvm::AllocaInst*> locals;
    FunctionBuilder(const FunctionNode* function, const Path& path, LLVMParser& parser)
        : functionNode(function), path(path), ctx(parser.ctx), mod(parser.mod), builder(parser.builder), symbolScanner(parser.symbolScanner){}

    llvm::Type* GetLLVMType(Vartype t);
        
    void Build();
};