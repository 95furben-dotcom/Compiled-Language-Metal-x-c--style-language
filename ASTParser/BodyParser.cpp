#include "AST.hpp"
#include <string>
#include <sys/types.h>
#include "../Tokenizer/Tokens.hpp"
#include "ASTParser.hpp"
#include "Debug/ASTError.hpp"
#include <iostream>

Result<BodyNode*> ASTParser::GetBody(){
    TRY(expectSign('{'));
    BodyNode body;
    while(true){
        if(isSign('}'))
            break;
        
        // will go on until } sign
        body.statements.push_back(TRY(GetStatementNode()));
    }
    TRY(expectSign('}'));
    return new BodyNode(body);
}

Result<ForNode*> ASTParser::GetForNode(){
    TRY(getToken<forToken>());

    ForNode* fornode = new ForNode();
    TRY(expectSign('('));

    // get var def
    uint specifiers = TRY(GetSpesifiers());
    std::optional<TypeNode> type = TRY(GetType());
    if(!type)
        return Fail("Expected type",index);

    auto varname = TRY(getToken<definisionToken>());
    fornode->iteratordef = TRY(GetVarDefNode(varname->name, type.value(), specifiers));
    // get condition
    ExprNode *condition = TRY(GetExprNode(0));

    TRY(expectEndl());
    // get assignamentnode
    ExprNode *target = TRY(GetExprNode(0)); 

    if(is<iterateToken>()){
        ADVANCE();
        fornode->assignement =  new AssignNode(target, new NumberNode("1",PrimitiveType::INT), '+');
    }else{
        char op = TRY(expectAssign());
        ExprNode *value = TRY(GetExprNode(0));
        fornode->assignement = new AssignNode(target, value, op);
    }
    TRY(expectSign(')'));
    fornode->loop = new WhileNode(condition, TRY(GetBody()));
    return fornode;
}


Result<StatementNode*> ASTParser::GetStatementNode(){
    std::cout << "Getting statement at " << index<<std::endl;
    // return
    if(is<specifierToken>()){
        uint specifiers = TRY(GetSpesifiers());
        auto type = TRY(getToken<typeToken>())->type;
        std::string name = TRY(expectDefinision());
        return GetVarDefNode(name, type, specifiers);
    }
    if(is<returnToken>()){
        ADVANCE(); // consume return
        ReturnNode *n = new ReturnNode();
        if (!is<endlToken>())
        n->value = TRY(GetExprNode(0));
        TRY(expectEndl());
        return n;
    }
    if(is<breakToken>()){
        ADVANCE();
        TRY(expectEndl());
        return new BreakNode();
    }
    if(is<continueToken>()){
        ADVANCE();
        TRY(expectEndl());
        return new ContinueNode();
    }
    if(is<ifToken>()){
        ADVANCE();
        TRY(expectSign('('));
        ExprNode *contition = TRY(GetExprNode(0));
        TRY(expectSign(')'));

        BodyNode* body = TRY(GetBody());
        BodyNode* elseBodyNode = nullptr;
        if(is<elseToken>()){
            elseBodyNode = TRY(GetBody());
        }

        return new IfNode(contition, body, elseBodyNode);
    }
    if(is<whileToken>()){
        ADVANCE();
        TRY(expectSign('('));
        ExprNode *contition = TRY(GetExprNode(0));
        TRY(expectSign(')'));

        BodyNode* body = TRY(GetBody());
        return new WhileNode(contition, body);
    }
    if(is<forToken>()){
       return GetForNode();
    }

    // primitive var def: int x = 3;
    if(auto t = tryGet<typeToken>()){
        VarDefNode* n = new VarDefNode(t->type);
        ADVANCE(); // consume type 
        n->name = TRY(expectDefinision()); // consumes varname
        if(is<endlToken>()){
            n->value = nullptr;
        }
        else{
            char c = TRY(expectAssign());
            if(c!='=') 
                return Fail("Expected '='",index);
            n->value = TRY(GetExprNode(0));
        }
        TRY(expectEndl());
        return n;
    }

    // struct var def or assignment
    if(auto t = tryGet<definisionToken>()){

        std::string firstname = t->name;
        // struct var def: Triangle shape;
        if(isStructType(firstname) || isNamespaceName(firstname)){
            if(isSign('.')){ 
                ADVANCE();
                // assignment: x = 3 or shape.width = 3 or funccall
                auto target = TRY(FindNextExprNode());
                if(auto a = dynamic_cast<FuncCallNode*>(target))
                    return a;

                char op = TRY(expectAssign());
                ExprNode *value = TRY(GetExprNode(0));
                AssignNode* an = new AssignNode(target, value, op);
                TRY(expectEndl());
                return an;
            }
            std::optional<TypeNode> type = TRY(GetType());
            if(!type) return Fail("Expected type",index);
            VarDefNode* n = new VarDefNode(type.value());
            n->name = TRY(expectDefinision());
            if(is<endlToken>()){
                n->value = nullptr;
            }
            else{
                char c = TRY(expectAssign());
                if(c!='=') return Fail("Expected '='",index);
                n->value = TRY(GetExprNode(0));
            }
            TRY(expectEndl());
            return n;
        }

        // reassignment: x = 3 or shape.width = 3 or funccall
        ExprNode *target = TRY(GetExprNode(0));
        if (FuncCallNode *a = dynamic_cast<FuncCallNode *>(target)) {
            TRY(expectEndl());
            return a;
        }   
        if(is<iterateToken>()){
            ADVANCE();
            TRY(expectEndl());
            return new AssignNode(target, new NumberNode("1",PrimitiveType::INT), '+');
        }
        char op = TRY(expectAssign());
        ExprNode *value = TRY(GetExprNode(0));
        AssignNode* an = new AssignNode(target, value, op);
        TRY(expectEndl());
        return an;
    }

    auto& tok = *tokens[index];
    return Fail(std::string("Statment cant start with ")+typeid(tok).name(),index);
}

Result<FuncCallParamsNode*> ASTParser::GetFuncCallParams(){
    if(isSign(')')){
        ADVANCE();
        return {};
    }
    FuncCallParamsNode* params = new FuncCallParamsNode();

    while(true){
        params->params.push_back(TRY(GetExprNode(0)));
        if(isSign(')'))
            break;
        TRY(expectSign(','));

    }
    TRY(expectSign(')'));
    return params;
}

