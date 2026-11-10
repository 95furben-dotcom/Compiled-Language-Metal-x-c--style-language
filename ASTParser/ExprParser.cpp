#include "ASTParser.hpp"
#include "../Tokenizer/TokenPrinter.hpp"
#include "Debug/ASTError.hpp"
Result<uint> ASTParser::OprandBind(std::string oprand){
    if(oprand == "==" || oprand == ">=" 
    || oprand == "<=" || oprand == "<" || oprand == ">"){
        return 1;
    }
    if(oprand == "+" || oprand == "-"){
        return 2;
    }
    if(oprand == "*" || oprand == "/" || oprand == "%"){
        return 3;
    }
    if(oprand == "^"){
        return 4;
    }
    return Fail("Invalid oprand '"+oprand+"'", index);
}


Result<ExprNode*> ASTParser::GetExprNode(uint bind){
    ExprNode* lhs = TRY(FindNextExprNode());

    // cout<<"found lhs at "<<index<<endl;

    while (true){
        if(is<endlToken>())
            return lhs;
        if(isSign(')') || isSign(',') || isSign(']') || is<assignmentToken>() || is<iterateToken>())
            return lhs;

        if(isSign('[')){
            ADVANCE();
            ExprNode *rhs = TRY(GetExprNode(0));
            TRY(expectSign(']'));
            lhs = new OprandNode("[]",lhs, rhs);
            continue;
        }
        std::cout << "GetExprNode: index=" << index
     << ", token=" << TokenPrinter::printToken(*tokens[index].get()) << std::endl;
        auto *op = tryGet<operatorToken>();
        if(!op) return Fail("Expected operatorToken in GetExprNode",index);
        auto newBind = TRY(OprandBind(op->s));

        if(newBind <= bind)
            return lhs;
        ADVANCE();

        ExprNode* rhs = TRY(GetExprNode(newBind));

        OprandNode* n = new OprandNode(op->s, lhs, rhs);
        lhs = n;
    }
    return lhs;
}

Result<ExprNode*> ASTParser::GetInstanceMemberNode(ExprNode* parentObject){
    const definisionToken *n = TRY(getToken<definisionToken>());
    std::string defname = n->name;
    if(isNamespaceName(defname) || isStructType(defname))
        return Fail("GetInstanceMemberNode called on type or namespace",index);

    // variable or funcall
    if(
        is<endlToken>()
        || is<assignmentToken>()
        || isSign(',')
        || isSign(')')
        || is<operatorToken>()
    ){
        // variable 
        return new InstanceMemberNode(parentObject, defname);
    }

    if(isSign('.')){
        ADVANCE(); // consume .
        InstanceMemberNode* instanceNode = new InstanceMemberNode(parentObject, defname);
        return GetInstanceMemberNode(instanceNode);
    }


    InstanceFuncCallNode* funcNode = new InstanceFuncCallNode(parentObject, defname);
    TRY(expectSign('('));
    funcNode->params = TRY(GetFuncCallParams());
    if(isSign('.')){
        ADVANCE();
        return GetInstanceMemberNode(funcNode);
    }
    return funcNode;
}

Result<ExprNode*> ASTParser::GetStaticMemberNode(std::vector<std::string>& path){
    auto n = TRY(getToken<definisionToken>());
    std::string defname = n->name;
    path.push_back(defname);

    if(isNamespaceName(defname) || isStructType(defname)){ 
        TRY(expectSign('.'));
        return GetStaticMemberNode(path);
    }
    
    if(
        is<endlToken>()
        || is<assignmentToken>()
        || isSign(',')
        || isSign(')')
        || is<operatorToken>()
    ){
        // variable 
        return new StaticMemberNode(path); 
    }
    // is token is sign
    if(isSign('.')){
        ADVANCE();// consume .
        const definisionToken *t = TRY(getToken<definisionToken>()); // we dont want to consume
        if(isNamespaceName(t->name) || isStructType(t->name)){
            return GetStaticMemberNode(path);
        }
        StaticMemberNode* varnode = new StaticMemberNode(path); 
        return GetInstanceMemberNode(varnode);
    }

    // must be function
    StaticFuncCallNode* funcNode = new StaticFuncCallNode(path);
    TRY(expectSign('('));
    // function
    // get params
    FuncCallParamsNode* params = TRY(GetFuncCallParams());
    funcNode->params = params;
    if(isSign('.')){
        return GetInstanceMemberNode(funcNode);
    }
    return funcNode;

}

Result<ExprNode*> ASTParser::FindNextExprNode(){
    // value, fuccall, valcall, parenthesis
    if(auto op = tryGet<operatorToken>()){
        ADVANCE();
        if(op->s != "-")
            return Fail("Expected '-' for exprnode",index);

        const valueToken *t = TRY(getToken<valueToken>());
        NumberNode* n = new NumberNode("-"+t->s, t->type);

        return n;
    }
    if (auto t = tryGet<valueToken>()){
        NumberNode* n = new NumberNode(t->s, t->type);
        
        ADVANCE(); // consume value
        return n;
    }
    if(auto t = tryGet<definisionToken>()){
        ADVANCE(); // consume defname 
        std::string defname = t->name;

        if(
            is<endlToken>()
            || is<assignmentToken>()
            || isSign(',')
            || isSign(')')
            || is<operatorToken>()
            || isSign('[')
            || isSign(']')
            || is<iterateToken>()
        ){
            // variable 
            return new VariableCallNode(defname);
        }
        // is token is sign
        if(isSign('.')){
            ADVANCE();// consume .
            if(isNamespaceName(defname) || isStructType(defname)){
                std::vector<std::string> path = {defname};
                return GetStaticMemberNode(path);
            }
            VariableCallNode* varCallNode = new VariableCallNode(defname);
            return GetInstanceMemberNode(varCallNode);
        }
        // is sign, but not '.'
        TRY(expectSign('('));
        FuncCallNode* funcNode = new FuncCallNode(defname);
        funcNode->funcname = defname;
        // function
        // get params
        FuncCallParamsNode* params = TRY(GetFuncCallParams());
        funcNode->params = params;
        if(isSign('.')){
            ADVANCE();
            return GetInstanceMemberNode(funcNode);
        }
        return funcNode;
    }
    if(isSign('(')){
        ExprNode* inside = TRY(GetExprNode(0));
        TRY(expectSign(')'));
        return inside;
    }
    if (auto s = tryGet<stringToken>()){
        StringNode* n = new StringNode(s->s);
        
        ADVANCE(); // consume value
        return n;
    }

    auto& tok = *tokens[index];
    return Fail(std::string("Invalid token in exprnode ")+typeid(tok).name(),index);
}

