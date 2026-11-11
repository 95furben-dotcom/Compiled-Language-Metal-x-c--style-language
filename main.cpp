#include "FileHandler.hpp"
#include "LLVMParser/LLVMBuilder.hpp"
#include "Tokenizer/Tokenizer.hpp"
#include "Tokenizer/TokenPrinter.hpp"

#include "ASTParser/ASTParser.hpp"
#include "ASTParser/Debug/ASTPrinter.hpp"

#include "LLVMParser/Symbolscanner/SymbolScanner.hpp"
#include "LLVMParser/Symbolscanner/SymbolPrinter.hpp"
#include <iostream>
#include <stdexcept>

int main(){
    //  try {
        std::string fileconts = ReadFile("ProjectFolder/printf-example.cl");
        auto tokens = Tokenizer::tokenize(fileconts, 0);
        TokenPrinter::tokensToFile(tokens, "ProjectFolder/.cache/tokens.txt");

        NamespaceNode *program = ASTParser::TokensToProgram(tokens);
        ASTPrinter::writeToFile(program, "ProjectFolder/.cache/AST.txt");
        if(ASTPrinter::CompileFailed(program)){
            ASTPrinter::print(program);
            throw std::runtime_error("AST Failded");
        }
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
        LLVMBuilder llvmbuilder(program);
        llvmbuilder.BuildStaticVariables();
        llvmbuilder.DeclareFunctions();
        llvmbuilder.BuildNamespace(program);
        llvmbuilder.mod.print(llvm::errs(), nullptr);
        llvmbuilder.WriteToFile("ProjectFolder/.cache/LLVMOutput.ll");

        if(std::system("clang ProjectFolder/.cache/LLVMOutput.ll -o program") == 0)
            std::system("./program; echo Return Value: $?");

    // } catch (const std::exception& e) {
    //     std::cerr << "error: " << e.what() << "\n";
    //     return 1;
    // }
}