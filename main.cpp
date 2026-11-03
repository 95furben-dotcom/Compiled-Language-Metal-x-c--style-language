#include "FileHandler.hpp"
#include "Tokenizer/Tokenizer.hpp"
#include "Tokenizer/TokenPrinter.hpp"

#include "ASTParser/ASTParser.hpp"
#include "ASTParser/Debug/ASTPrinter.hpp"

#include "LLVMParser/Symbolscanner/SymbolScanner.hpp"
#include "LLVMParser/Symbolscanner/SymbolPrinter.hpp"

int main(){
    std::string fileconts = ReadFile("ProjectFolder/goal.cl");
    auto tokens = Tokenizer::tokenize(fileconts, 0);
    TokenPrinter::tokensToFile(tokens, "ProjectFolder/.cache/tokens.txt");

    NamespaceNode *program = ASTParser::TokensToProgram(tokens);
    ASTPrinter::writeToFile(program, "ProjectFolder/.cache/AST.txt");

    SymbolScanner s(program);
    printf("\nSymbolScannerFound{\n");
    printf("\tfunctionEntries: %zu\n",s.functionEntries.size());
    printf("\tstructEntries: %zu\n",s.structEntries.size());
    printf("\tsaticVariableEntries: %zu\n",s.staticVariableEntries.size());
    printf("\tkernelEntries: %zu\n",s.kernelEntries.size());
    printf("}\n");
    // EntryPrinter::printAll(s.functionEntries);
    // EntryPrinter::printAll(s.structEntries);
    // EntryPrinter::printAll(s.staticVariableEntries);
    EntryPrinter::printAll(s.kernelEntries);
    EntryPrinter::writeToFile(s, "ProjectFolder/.cache/SymbolEntries.txt");
}