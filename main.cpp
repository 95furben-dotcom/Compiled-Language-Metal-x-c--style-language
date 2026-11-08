#include "FileHandler.hpp"
#include "LLVMParser/LLVMBuilder.hpp"
#include "Tokenizer/Tokenizer.hpp"
#include "Tokenizer/TokenPrinter.hpp"

#include "ASTParser/ASTParser.hpp"
#include "ASTParser/Debug/ASTPrinter.hpp"

#include "LLVMParser/Symbolscanner/SymbolScanner.hpp"
#include "LLVMParser/Symbolscanner/SymbolPrinter.hpp"

int main(){
     try {
        std::string fileconts = ReadFile("ProjectFolder/main.cl");
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
        LLVMBuilder llvmbuilder(program);
        llvmbuilder.BuildStaticVariables();
        llvmbuilder.DeclareFunctions();
        llvmbuilder.BuildNamespace(program);
        llvmbuilder.WriteToFile("ProjectFolder/.cache/LLVMOutput.ll");

        if(std::system("clang ProjectFolder/.cache/LLVMOutput.ll -o program") == 0)
            std::system("./program; echo $?");

    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}