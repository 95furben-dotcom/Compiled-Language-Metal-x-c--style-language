#include "LLVMParser.hpp"
#include "Utillity/LLVMDebug.hpp"
#include <map>

struct LLVMParser::FunctionBuilder
{ 
    const FunctionNode* functionNode;
    Path path;
    LLVMParser& parser;

    std::map<std::string, llvm::AllocaInst*> locals;
    FunctionBuilder(const FunctionNode* function, const Path& path, LLVMParser& parser)
        : functionNode(function), path(path), parser(parser){}

    void Build();
};