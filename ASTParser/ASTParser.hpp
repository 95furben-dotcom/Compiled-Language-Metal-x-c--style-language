#pragma once
#include "AST.hpp"
#include "../Tokenizer/Tokens.hpp"
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include "Debug/ASTError.hpp"
#include "Specifiers.hpp"

struct ASTParser{
    const std::vector<std::unique_ptr<Token>>& tokens;
    uint& index;

    const std::vector<std::string> structNames;
    const std::vector<std::string> namespaceNames;
    const std::unordered_map<std::string, uint> OprandBindes{
        {"<",1},{">",1},{"<=",1},{">=",1},{"==",1},
        {"+",2},{"-",2},
        {"*",3},{"/",3}
    };
    template<typename T>
    Result<const T*> getToken(){
        static_assert(std::is_base_of<Token, T>(), "T must be a token");
        const T* t = dynamic_cast<const T*>(tokens[index].get());
        if(t == nullptr){
            auto& tok = *tokens[index];
            std::string s = std::string(typeid(T).name())+", got: "+typeid(tok).name();
            return Fail("Expected token: "+s, index);
        }
        ADVANCE();
        return t;
    }

    Result<void> expectSign(char c){
        if(!isSign(c)){ 
            return Fail("Expected sign"+std::string(1,c), index);
        } 
        ADVANCE();
        return {};

    }

    Result<void> expectEndl(){
        TRY_VOID(getToken<endlToken>());
        return {};
    }
    Result<char> expectAssign(){
        const assignmentToken *a = TRY(getToken<assignmentToken>());
        return a->c;
    }
    Result<std::string> expectDefinision(){
        const definisionToken *a = TRY(getToken<definisionToken>());
        return a->name;
    }
    
    template<typename T>
    T* tryGet() {
        return dynamic_cast<T*>(tokens[index].get());
    }
    template<typename T>
    bool is() {
        auto t = dynamic_cast<const T*>(tokens[index].get());
        return t != nullptr;
    }
    bool isSign(char sign) {
        auto t = dynamic_cast<const signToken*>(tokens[index].get());
        return t != nullptr && t->c==sign;
    }


    inline Result<void> AdvanceToSign(char c){
        while(true){
            ADVANCE();
            if(isSign(c)){
                return {};
            }
        }
    }

    // body parser
    Result<StatementNode*> GetStatementNode();
    Result<BodyNode*> GetBody();
    Result<ForNode*> GetForNode();
    Result<FuncCallParamsNode*> GetFuncCallParams();
    
    // expr parser
    Result<uint> OprandBind(std::string oprand);
    Result<ExprNode*> GetInstanceMemberNode(ExprNode* parentObject);
    Result<ExprNode*> GetStaticMemberNode(std::vector<std::string>& path);
    Result<ExprNode*> FindNextExprNode();
    Result<ExprNode*> GetExprNode(uint bind);

    // operational 
   

    static std::vector<std::string> findStructNames(const std::vector<std::unique_ptr<Token>>& tokens){
        std::vector<std::string> structNames;
        bool lastWasSturctToken = false;
        for(auto& t : tokens){
            auto deft = dynamic_cast<const definisionToken*>(t.get());
            if(lastWasSturctToken && deft != nullptr){
                structNames.push_back(deft->name);
            }

            auto structt = dynamic_cast<const structToken*>(t.get());
            lastWasSturctToken = structt != nullptr;
        }
        return structNames;
    }

    static std::vector<std::string> findnamespaceNames(const std::vector<std::unique_ptr<Token>>& tokens){
        std::vector<std::string> namespaceNames;
        bool lastWasSturctToken = false;
        for(auto& t : tokens){
            auto deft = dynamic_cast<const definisionToken*>(t.get());
            if(lastWasSturctToken && deft != nullptr){
                namespaceNames.push_back(deft->name);
            }

            auto structt = dynamic_cast<const namespaceToken*>(t.get());
            lastWasSturctToken = structt != nullptr;
        }
        return namespaceNames;
    }


    ASTParser(const std::vector<std::unique_ptr<Token>>& tokens, uint& index) 
    : tokens(tokens), index(index), structNames(findStructNames(tokens)), namespaceNames(findnamespaceNames(tokens)){}

    

    bool isStructType(std::string s){
        return (
            std::find(
                    structNames.begin(), 
                    structNames.end(), 
                    s
                ) 
            != structNames.end()
        );
    }

    bool isNamespaceName(std::string s){
        return (
            std::find(
                    namespaceNames.begin(), 
                    namespaceNames.end(), 
                    s
                ) 
            != namespaceNames.end()
        );
    }


    Result<TypeNode> GetType(){
        if(auto t = tryGet<typeToken>()){
            ADVANCE(); // consume type 
            return t->type;
        }
        else{
            auto def = TRY(getToken<definisionToken>());
            std::vector<std::string> structPath{def->name};

            while (isSign('.')) {
                ADVANCE(); // consume .
                auto d = TRY(expectDefinision());
                structPath.push_back(d);
            }
            return Vartype::NewStructType(structPath);

        }
    }


    Result<VarDefNode*> GetVarDefNode(std::string name, Vartype type, uint specifiers){
        VarDefNode* n = new VarDefNode(type, name, nullptr, specifiers);

        if(is<endlToken>()){
            n->value = nullptr;
        }
        else{
            TRY(getToken<assignmentToken>()); // expect assignment
            n->value = TRY(GetExprNode(0));
        }
        TRY(expectEndl());
        return n;
    }


    
    Result<FunctionNode*> GetFunctionNode(std::string name, Vartype type, uint specifiers){
        FunctionNode* fn = new FunctionNode(type, name, specifiers);
        TRY(expectSign('('));
        if(auto t = tryGet<signToken>())
        {
            if(t->c != ')')
                return Fail("Got sign, expected ')' or param",index);
            ADVANCE(); // consume )
            fn->params = {};
        }
        else{
            // now, get params
            std::vector<VarDefNode*> params;
            while(true){
                uint specifier = TRY(GetSpesifiers());
                TypeNode paramType = TRY(GetType());

                auto name = TRY(expectDefinision());

                VarDefNode* pn = new VarDefNode(paramType, name, nullptr, specifier);
                params.push_back(pn);

                auto t = TRY(getToken<signToken>());
                if(t->c == ',')
                    continue;
                        if(t->c == ')')
                    break;
                        return Fail("No ",index);  
            }   
            fn->params = params;
        }

        // get body
        fn->body = TRY(GetBody());

        return fn;    
    }

    

    

    Result<StructNode*> GetStructNode(){
        if(!is<structToken>())
            return Fail("struct token not found THIS ERROR SHOULD NOT BE SHOWN IF PROGRAM IS CORRECT", index);
        ADVANCE();

        StructNode* sn = new StructNode(TRY(expectDefinision()));
        TRY(expectSign('{'));

        while(!isSign('}')){
            sn->members.push_back(TRY(GetMemberNode()));
        }
        TRY(expectSign('}'));
        return sn; 
    }

    Result<uint> GetSpesifiers(){
        uint specifiers = 0;

        while(auto n = tryGet<specifierToken>()){
            ADVANCE();
            if(index >= tokens.size())
                return 0;
            if(n->s == "static")
                specifiers |= Specifiers::STATIC;
            else if(n->s == "const")
                specifiers |= Specifiers::CONST;
            else if(n->s == "inline")
                specifiers |= Specifiers::INLINE;
            else if(n->s == "kernel")
                specifiers |= Specifiers::KERNEL;
            else if(n->s == "_gpu")
                specifiers |= Specifiers::GPUSTORED;
            else throw std::runtime_error("Unknown specifier: \""+n->s+"\"");
        }
        return specifiers;
    }

    Result<KernelDefinitionNode*> GetKernelDefinitionNode(const std::string& name){
        TRY(expectSign('('));
        KernelDefinitionNode* fn = new KernelDefinitionNode(name);
        // get params 
        if(auto t = tryGet<signToken>())
        {
            if(t->c != ')')
                return Fail("Expected param or ')', got other sign", index);
            ADVANCE(); // consume )
            fn->params = {};
        }
        else{
            // now, get params
            std::vector<VarDefNode*> params;
            while(true){
                uint specifier = TRY(GetSpesifiers());
                TypeNode type = TRY(GetType());

                if(!is<definisionToken>())
                    return Fail("Expected definision token",index);
                std::string name = TRY(expectDefinision());

                VarDefNode* pn = new VarDefNode(type, name, nullptr, specifier);
                params.push_back(pn);

                auto t = TRY(getToken<signToken>());
                if(t->c == ',')
                    continue;
                        if(t->c == ')')
                    break;
                        return Fail(std::string("Epected ',' or ')', got ")+t->c, index);  
            }   
            fn->params = params;
        }

        // get body
        fn->body = TRY(GetBody());

        return fn;    
    }

    Result<MemberNode*> GetMemberNode(){
        // function,  struct or variable definitions 
        if(is<structToken>()){
            return TRY(GetStructNode());
        }
        else{
            uint specifiers = TRY(GetSpesifiers());
            TypeNode type = TRY(GetType());
            std::cout << "Got type, current index: "<<index<<"\n";
            std::string defname = TRY(expectDefinision());
            std::cout << "Got defname\n";

            if((specifiers&Specifiers::KERNEL) != 0)
                return GetKernelDefinitionNode(defname);

            auto sign = tryGet<signToken>();
            if(sign != nullptr && sign->c == '('){
                return GetFunctionNode(defname, type, specifiers);
            }
            return GetVarDefNode(defname, type, specifiers);
        }
    }

    Result<DeclNode*> GetDeclNode(){
        // entries: namespace, functions, structs or variable definitions 
        if(is<namespaceToken>())
            return TRY(GetNamespaceNode());
        else
            return TRY(GetMemberNode());
        
    }

    Result<NamespaceNode*> GetNamespaceNode(){
        TRY(getToken<namespaceToken>());


        NamespaceNode* nn = new NamespaceNode();
        nn->name = TRY(expectDefinision());
        TRY(expectSign('{'));
        while(index<tokens.size() && !isSign('}')){
            nn->declarations.push_back(TRY(GetDeclNode()));
        }
        TRY(expectSign('}'));

        return nn; 
    }

    
    
    static NamespaceNode* TokensToProgram(const std::vector<std::unique_ptr<Token>>& tokens){
        uint index = 0;
        
        ASTParser parser(tokens,index);
        NamespaceNode* program = new NamespaceNode();
        while(index<tokens.size()){
            uint lastIndex = index;
            Result<DeclNode *> a = parser.GetDeclNode();
            program->declarations.push_back(Ensure(a));
            if(lastIndex == index)
                break;
        } 
        program->name = "program";
        return program;
    }
};