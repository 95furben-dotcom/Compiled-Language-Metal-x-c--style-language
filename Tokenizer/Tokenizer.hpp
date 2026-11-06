#pragma once
#include <cctype>
#include <optional>
#include <string>
#include <vector>
#include <memory>

#include "Tokens.hpp"

namespace Tokenizer{

    const std::string signs = "()[]{},."; 
    const std::string operators = "+-=*/%<>";
    const std::vector<std::string> specifiers = {"static","const","inline","_gpu","kernel"};
    
    class tokenize_error : public std::exception {
    public:
        std::string message;

        tokenize_error(const std::string& msg) : message(msg) {}

        const char* what() const noexcept override {
            return message.c_str();
        }
    };

    inline bool isAlphaChar(const char& c){
        return isalpha(c) || c == '_';
    }

    static std::string getDefinition(const std::string &fileconts, uint &index) {
      uint startIndex = index;
      while (isdigit(fileconts[index]) || isAlphaChar(fileconts[index])) {
        index++;
      }
      return fileconts.substr(startIndex, index - startIndex);
    }

    static std::optional<std::unique_ptr<typeToken>> tryGetType(const std::string &fileconts, uint &index){
        uint endIndex = index;
        std::string subject = getDefinition(fileconts, endIndex);
        if(Vartype::IsStringPrimitive(subject)){
            index = endIndex;
            return std::make_unique<typeToken>((stringToPrimitiveType(subject)));
        }
        if(subject == "string"){
            index = endIndex;
            return std::make_unique<typeToken>(STRINGTYPE);
        }
        if(subject == "vector"){    
            index = endIndex;
            if(fileconts[index] != '<')
                throw tokenize_error("expects < after vector");
            index++;

            auto templateType = tryGetType(fileconts, index);
            if(!templateType.has_value())
                throw tokenize_error("Could not get rest of vectortype");
            
            if(fileconts[index] != '>')
                throw tokenize_error("expects > after vector");
            index++;

            return std::make_unique<typeToken>(
                Vartype::NewVectorType(
                    new Vartype(templateType.value()->type)));
            
        }
        return {};
    }


    static std::unique_ptr<Token> getToken(const std::string& fileconts, uint& index){
        const char& c = fileconts[index];   // can not be space
        if(c == ';'){
            index++;
            return std::make_unique<endlToken>();
        }

        if(signs.contains(c)){
            index++;
            return std::make_unique<signToken>(c);
        }

        if(operators.contains(c)){
            index++; // consume operator
            auto next = fileconts[index];
            if(next == '='){
                index++;
                if(c == '='){
                    index++;
                    return std::make_unique<operatorToken>("==");
                }
                return std::make_unique<assignmentToken>(c);
            }
            if(c == '=')
                return std::make_unique<assignmentToken>('=');

            if(operators.contains(next)){
                std::string op = std::string({c,next});
                index++;
                if(op == "++"){
                    return std::make_unique<iterateToken>(true);
                }
                if(op == "--"){
                    return std::make_unique<iterateToken>(false);
                }
                return make_unique<operatorToken>(std::string(1,c)+next);

            }
            else return make_unique<operatorToken>(std::string(1,c));
        }

        // value detection 
        if(isdigit(c)){

            uint startIndex = index;
            while(isdigit(fileconts[index]) || fileconts[index] == '.' || isalpha(fileconts[index])){
                index++;
            } 
            return make_unique<valueToken>(fileconts.substr(startIndex, index-startIndex));
        }

        if(isAlphaChar(c)){
            // check all values and names
            // continue till no more valid characters 
            if(auto typeToken = tryGetType(fileconts, index)){
                if(typeToken.value()==nullptr)
                    throw tokenize_error("tryGetType returned value null");
                return std::move(typeToken.value());
            }
            
            std::string subject = getDefinition(fileconts, index);

            if(subject == "return"){
                return std::make_unique<returnToken>();
            }
            if(subject == "struct"){
                return std::make_unique<structToken>();
            }
            if(subject == "namespace"){
                return std::make_unique<namespaceToken>();
            }
            if(subject == "true"){
                return make_unique<valueToken>(subject);
            }
            if(subject == "false"){
                return make_unique<valueToken>(subject);
            }
            if(subject == "if"){
                return std::make_unique<ifToken>();
            }
            if(subject == "else"){
                return std::make_unique<elseToken>();
            }
            if(subject == "while"){
                return std::make_unique<whileToken>();
            }
            if(subject == "for"){
                return std::make_unique<forToken>();
            }
            if(subject == "continue"){
                return std::make_unique<continueToken>();
            }
            if(subject == "break"){
                return std::make_unique<breakToken>();
            }

            if(find(specifiers.begin(), specifiers.end(), subject)!=specifiers.end()){
                return make_unique<specifierToken>(subject);
            }

            // it must be a self defined var
            return make_unique<definisionToken>(subject);
        }

        if(c == '"'){
            index++; // consume "
            // generate string token
            std::string content = "";
            bool nextIsSlashed = false;
            while(nextIsSlashed || fileconts[index] != '"'){
                if(nextIsSlashed) nextIsSlashed = false;
                else if(fileconts[index] == '\\') nextIsSlashed = true;
                content += fileconts[index];
                index++;
                if(index > fileconts.size() - 1)
                    throw tokenize_error("String never finished");

            }
            index++; // consume "
            return make_unique<stringToken>(content);
        }
        throw tokenize_error(std::string("Could not find token for \'") + c+"\'");
    }


    static std::vector<std::unique_ptr<Token>> tokenize(std::string fileconts, uint offset){ // offset is byond definisjons 
        uint lineIndex = 0;

        int lastNewLine = -1;
        std::vector<std::unique_ptr<Token>> tokens;
        uint index = offset; 

        auto RemoveSpace = [&fileconts, &index, &lastNewLine, &lineIndex](){
            while (isspace(fileconts[index])){
                if(fileconts[index] == '\n'){
                    lastNewLine = index;
                    lineIndex++;
                }
                index++;
            } 
        };

        RemoveSpace();

        while(index < fileconts.size()){
            while(fileconts[index] == '/' && fileconts[index + 1] == '/'){
                while(fileconts[index] != '\n') index++;
                RemoveSpace();
            }
            try{
            std::unique_ptr<Token> t = getToken(fileconts, index);
                t->lineIndex = lineIndex;
                t->charIndex = index - lastNewLine - 1;
                tokens.push_back(std::move(t));
                RemoveSpace();
            }
            catch (tokenize_error e){
                printf("%s",e.what());
                return tokens;
            }
           
        }
        return tokens;
    }
}