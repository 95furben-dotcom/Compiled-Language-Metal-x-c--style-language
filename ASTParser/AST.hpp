#pragma once
#include "Debug/ParseError.hpp"
#pragma once
// base node
#include <string>
#include <vector>
#include "../Vartype.hpp"





struct Node {
    virtual ~Node() = default;
    ParseError* error = nullptr;

    bool isError() const{
        return error;
    }
};


struct ExprNode : virtual Node {};       // things that produce a value: 1+2, foo(), x
struct StatementNode : virtual Node {};  // things inside a function body: return, if, assignment
struct DeclNode : virtual Node {};       // top level declarations
struct MemberNode : virtual Node, virtual DeclNode {};     // things inside a struct: variables, functions

struct TypeNode : virtual Node, Vartype{
    TypeNode(const Vartype& t) : Vartype(t){}
    TypeNode() : Vartype(PrimitiveType::VOID) {}
};

struct BodyNode : Node{
    BodyNode():statements(){};
    std::vector<StatementNode*> statements;
};

struct FuncCallParamsNode : virtual Node{
    FuncCallParamsNode():params(){};
    std::vector<ExprNode*> params;
};


struct NumberNode : ExprNode{
    NumberNode() : value(""), type(PrimitiveType::VOID) {}
    NumberNode(std::string value, const PrimitiveType type)
        : value(value), type(type){}

    std::string value;
    PrimitiveType type;
};

struct StringNode : ExprNode{
    StringNode() : value("") {}
    StringNode(std::string value)
        : value(value) {}
    std::string value;

};


struct OprandNode : ExprNode{
    OprandNode() : oprand(""), v1(nullptr), v2(nullptr) {}
    OprandNode(std::string oprand, ExprNode* v1, ExprNode* v2)
        : oprand(oprand), v1(v1), v2(v2) {}

    std::string oprand;
    ExprNode* v1 = nullptr;
    ExprNode* v2 = nullptr;
};

struct VariableCallNode : ExprNode{
    VariableCallNode() : name("") {}
    VariableCallNode(std::string name) : name(name) {}

    std::string name;
};

struct VarDefNode : StatementNode, MemberNode{
    VarDefNode() : type(), name(""), value(nullptr), specifiers(0) {}
    VarDefNode(TypeNode type, std::string name = "", ExprNode* value = nullptr, uint specifiers = 0)
        : type(type), name(name), value(value), specifiers(specifiers) {}

    TypeNode type;
    std::string name;
    ExprNode* value = nullptr;

    uint specifiers;
};

struct ReturnNode : StatementNode{
    ReturnNode(ExprNode* value = nullptr) : value(value) {}

    ExprNode* value = nullptr;
};

struct FunctionNode : MemberNode{
    FunctionNode() : returnType(), name(""), body(nullptr), specifiers(0) {}
    FunctionNode(TypeNode returnType, std::string name = "", uint specifiers = 0)
        : returnType(returnType), name(name), body(nullptr),
        specifiers(specifiers){}

    TypeNode returnType;
    std::string name;
    BodyNode* body;
    std::vector<VarDefNode*> params;

    uint specifiers;
};

struct FuncCallNode : ExprNode, StatementNode{
    FuncCallNode() : funcname(""), params(nullptr) {}
    FuncCallNode(std::string funcname, FuncCallParamsNode* params = {})
        : funcname(funcname), params(params) {}

    std::string funcname;
    FuncCallParamsNode* params;

    virtual std::vector<std::string> GetPath() const {
        return {funcname};
    }
};

struct NamespaceNode : DeclNode{
    NamespaceNode(std::string name = "") : name(name) {}

    std::string name;
    std::vector<DeclNode*> declarations;
};

struct StructNode : MemberNode{
    StructNode() : name("") {}
    StructNode(std::string name) : name(name) {}

    std::string name;
    std::vector<MemberNode*> members;
};

// A.x  — static/namespace variable access
struct StaticMemberNode : ExprNode {
    StaticMemberNode() : path() {}
    StaticMemberNode(std::vector<std::string> path) : path(path) {}

    std::vector<std::string> path; // {"A", "x"}
};

// Shapes.Triangle.makeDefault()  — static function call
struct StaticFuncCallNode : FuncCallNode {
    StaticFuncCallNode() : FuncCallNode("", {}), path() {}
    StaticFuncCallNode(std::vector<std::string> path, FuncCallParamsNode* params = {})
        : FuncCallNode(path.empty() ? "" : path.back(), params), path(path) {}

    std::vector<std::string> path; // {"Shapes", "Triangle", "makeDefault"}

    std::vector<std::string> GetPath() const override {
        return path;
    }
};

// shape.sum()  — instance method call
struct InstanceFuncCallNode : FuncCallNode {
    InstanceFuncCallNode() : FuncCallNode("", {}), object(nullptr) {}
    InstanceFuncCallNode(ExprNode* object, std::string funcname, FuncCallParamsNode* params = {})
        : FuncCallNode(funcname, params), object(object) {}

    ExprNode* object = nullptr;
};

struct InstanceMemberNode : ExprNode {
    InstanceMemberNode() : object(nullptr), field("") {}
    InstanceMemberNode(ExprNode* object, std::string field)
        : object(object), field(field) {}

    ExprNode* object = nullptr;   // the instance expression (shape)
    std::string field;            // "width"
};

struct AssignNode : StatementNode {        // shape.width = 3
    AssignNode() : target(nullptr), value(nullptr), op('=') {}
    AssignNode(ExprNode* target, ExprNode* value, char op = '=')
        : target(target), value(value), op(op) {}

    ExprNode* target = nullptr; // MemberExprNode or varvallnode
    ExprNode* value = nullptr;
    char op = '=';
};


// metal

struct ComputeRequestCall : FuncCallNode {
    ComputeRequestCall() : FuncCallNode("", {}), path() {}
    ComputeRequestCall(std::vector<std::string> path, FuncCallParamsNode* params = {})
        : FuncCallNode(path.empty() ? "" : path.back(), params), path(path) {}

    std::vector<std::string> path; // {"Shapes", "Triangle", "makeDefault"}

    std::vector<std::string> GetPath() const override {
        return path;
    }
};


struct KernelDefinitionNode : MemberNode{
    KernelDefinitionNode() : name(""), body(nullptr) {}
    KernelDefinitionNode(std::string name):name(name){}

    std::string name = "";
    BodyNode* body = nullptr;
    std::vector<VarDefNode*> params;
};
 

struct IfNode : StatementNode{
    IfNode() : condition(nullptr), body(nullptr), elseBody(nullptr) {}
    IfNode(ExprNode* condition, BodyNode* body, BodyNode* elseBody) 
        : condition(condition), body(body), elseBody(elseBody){};
    ExprNode* condition;
    BodyNode* body;

    BodyNode* elseBody;
};

struct WhileNode : StatementNode{
    WhileNode() : condition(nullptr), body(nullptr) {}
    WhileNode(ExprNode* condition, BodyNode* body) 
        : condition(condition), body(body){};
    ExprNode* condition;
    BodyNode* body;
};

struct ForNode : StatementNode{
    ForNode() : iteratordef(nullptr), assignement(nullptr), loop(nullptr) {}
    VarDefNode* iteratordef;
    AssignNode* assignement;
    WhileNode* loop;
};

struct ContinueNode : StatementNode {};
struct BreakNode    : StatementNode {};