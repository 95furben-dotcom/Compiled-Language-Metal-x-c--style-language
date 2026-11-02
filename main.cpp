#include "FileHandler.hpp"
#include "LLVMParser/Symbolscanner/SymbolScanner.hpp"
#include "Tokenizer/Tokenizer.hpp"
#include "Tokenizer/TokenPrinter.hpp"

#include "ASTParser/ASTParser.hpp"
#include "ASTParser/Debug/ASTPrinter.hpp"


int main(){
    std::string fileconts = ReadFile("ProjectFolder/goal.cl");
    auto tokens = Tokenizer::tokenize(fileconts, 0);
    TokenPrinter::tokensToFile(tokens, "ProjectFolder/.cache/tokens.txt");

    NamespaceNode *program = ASTParser::TokensToProgram(tokens);
    ASTPrinter::writeToFile(program, "ProjectFolder/.cache/AST.txt");

    Symbolscanner s(program);

}