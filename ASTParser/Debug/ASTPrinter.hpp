#pragma once
#include "../AST.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "../../Vartype.hpp"
#include "../Specifiers.hpp"

namespace ASTPrinter {

static void appendIndent(std::ostringstream& out, const std::vector<bool>& ls) {
    for (int i = 0; i < (int)ls.size() - 1; i++)
        out << (ls[i] ? "    " : "│   ");
    if (!ls.empty())
        out << (ls.back() ? "└── " : "├── ");
}

static std::string modifierPrefix(uint specifiers) {
    std::string out;
    if (specifiers & Specifiers::STATIC)    out += "static ";
    if (specifiers & Specifiers::CONST)     out += "const ";
    if (specifiers & Specifiers::INLINE)    out += "inline ";
    if (specifiers & Specifiers::GPUSTORED) out += "gpustored ";
    if (specifiers & Specifiers::KERNEL)    out += "kernel ";
    return out;
}

// Returns true and prints "ErrorNode: msg" if node carries an error.
// Caller should skip normal printing when this returns true.
static bool printIfError(std::ostringstream& out, const Node* node) {
    if (!node || !node->isError())
        return false;

    const ParseError& err = *node->error;

    out << "ErrorNode: \"" << err.message << "\""
        << " (token " << err.tokenIndex << ")\n";

    for (const Frame& f : err.trace) {
        out << "    in " << f.func
            << " (" << f.file << ":" << f.line << ")\n";
    }

    return true;
}

static void printExpr(std::ostringstream& out, const ExprNode* node, std::vector<bool> ls);
static void printStatement(std::ostringstream& out, const StatementNode* node, std::vector<bool> ls);
static void printMember(std::ostringstream& out, const MemberNode* node, std::vector<bool> ls);
static void printDecl(std::ostringstream& out, const DeclNode* node, std::vector<bool> ls);

// params is now FuncCallParamsNode* — this helper centralizes the null/deref handling
static size_t paramCount(const FuncCallParamsNode* params) {
    return params ? params->params.size() : 0;
}
static ExprNode* paramAt(const FuncCallParamsNode* params, size_t i) {
    return params->params[i];
}

static void printExpr(std::ostringstream& out, const ExprNode* node, std::vector<bool> ls) {
    appendIndent(out, ls);
    if (!node) { out << "(null)\n"; return; }
    if (printIfError(out, node)) return;

    if (auto* n = dynamic_cast<const NumberNode*>(node)) {
        out << "NumberNode [" << n->value << "]\n";

    } else if (auto* n = dynamic_cast<const StringNode*>(node)) {
        out << "StringNode \"" << n->value << "\"\n";

    } else if (auto* n = dynamic_cast<const OprandNode*>(node)) {
        out << "OprandNode \"" << n->oprand << "\"\n";
        auto c = ls; c.push_back(false); printExpr(out, n->v1, c);
        c.back() = true;                 printExpr(out, n->v2, c);

    } else if (auto* n = dynamic_cast<const InstanceMemberNode*>(node)) {
        out << "InstanceMemberNode \"" << n->field << "\"\n";
        auto c = ls; c.push_back(true); printExpr(out, n->object, c);

    } else if (auto* n = dynamic_cast<const StaticMemberNode*>(node)) {
        out << "StaticMemberNode \"" << join(n->path, '.') << "\"\n";

    } else if (auto* n = dynamic_cast<const ComputeRequestCall*>(node)) {
        out << "ComputeRequestCall \"" << join(n->path, '.') << "\"\n";
        size_t count = paramCount(n->params);
        for (size_t i = 0; i < count; i++) {
            auto c = ls; c.push_back(i == count - 1);
            printExpr(out, paramAt(n->params, i), c);
        }

    } else if (auto* n = dynamic_cast<const StaticFuncCallNode*>(node)) {
        out << "StaticFuncCallNode \"" << join(n->path, '.') << "\"\n";
        size_t count = paramCount(n->params);
        for (size_t i = 0; i < count; i++) {
            auto c = ls; c.push_back(i == count - 1);
            printExpr(out, paramAt(n->params, i), c);
        }

    } else if (auto* n = dynamic_cast<const InstanceFuncCallNode*>(node)) {
        out << "InstanceFuncCallNode \"" << n->funcname << "\"\n";
        size_t count = paramCount(n->params);
        size_t total = 1 + count;
        size_t idx = 0;
        auto c = ls; c.push_back(++idx == total);
        printExpr(out, n->object, c);
        for (size_t i = 0; i < count; i++) {
            auto cc = ls; cc.push_back(++idx == total);
            printExpr(out, paramAt(n->params, i), cc);
        }

    } else if (auto* n = dynamic_cast<const FuncCallNode*>(node)) {
        out << "FuncCallNode \"" << n->funcname << "\"\n";
        size_t count = paramCount(n->params);
        for (size_t i = 0; i < count; i++) {
            auto c = ls; c.push_back(i == count - 1);
            printExpr(out, paramAt(n->params, i), c);
        }
    } else if (auto* n = dynamic_cast<const VariableCallNode*>(node)) {
        out << "VariableCallNode \"" << n->name << "\"\n";
    } else {
        out << "UnknownExprNode\n";
    }
}

// BodyNode wraps .statements, and Function/If/While/Kernel hold BodyNode* (possibly null)
static void printBody(std::ostringstream& out, const BodyNode* body, std::vector<bool> ls) {
    if (!body) return; // nothing to print for a null body
    if (printIfError(out, body)) return;
    const auto& stmts = body->statements;
    for (int i = 0; i < (int)stmts.size(); i++) {
        auto c = ls; c.push_back(i == (int)stmts.size() - 1);
        printStatement(out, stmts[i], c);
    }
}

static void printStatement(std::ostringstream& out, const StatementNode* node, std::vector<bool> ls) {
    if (auto* n = dynamic_cast<const FuncCallNode*>(node)) {
        printExpr(out, n, ls);
        return;
    }

    appendIndent(out, ls);
    if (!node) { out << "(null)\n"; return; }
    if (printIfError(out, node)) return;

    if (auto* n = dynamic_cast<const ReturnNode*>(node)) {
        out << "ReturnNode\n";
        auto c = ls; c.push_back(true); printExpr(out, n->value, c);

    } else if (auto* n = dynamic_cast<const AssignNode*>(node)) {
        if (n->op == '=')
            out << "AssignNode \"" << n->op << "\"\n";
        else
            out << "AssignNode \"" << n->op << "=\"\n";
        auto c = ls; 
        c.push_back(false); 
        printExpr(out, n->target, c);
        c.back() = true;                 
        printExpr(out, n->value, c);

    } else if (auto* n = dynamic_cast<const VarDefNode*>(node)) {
        out << modifierPrefix(n->specifiers)
            << "VarDefNode \"" << n->name << "\" : "
            << vartypeToString(n->type) << "\n";
        if (n->value) { auto c = ls; c.push_back(true); printExpr(out, n->value, c); }

    } else if (auto* n = dynamic_cast<const IfNode*>(node)) {
        out << "IfNode\n";
        bool hasElse = n->elseBody != nullptr;
        bool thenEmpty = !n->body || n->body->statements.empty();

        {
            auto cc = ls; cc.push_back(thenEmpty && !hasElse);
            appendIndent(out, cc);
            out << "condition:\n";
            auto ccc = cc; ccc.push_back(true);
            printExpr(out, n->condition, ccc);
        }
        {
            auto cc = ls; cc.push_back(!hasElse);
            appendIndent(out, cc);
            out << "then:\n";
            auto ccc = cc; ccc.push_back(true);
            printBody(out, n->body, ccc);
        }
        if (hasElse) {
            auto cc = ls; cc.push_back(true);
            appendIndent(out, cc);
            out << "else:\n";
            auto ccc = cc; ccc.push_back(true);
            printBody(out, n->elseBody, ccc);
        }

    } else if (auto* n = dynamic_cast<const WhileNode*>(node)) {
        out << "WhileNode\n";
        {
            auto cc = ls; cc.push_back(false);
            appendIndent(out, cc);
            out << "condition:\n";
            auto ccc = cc; ccc.push_back(true);
            printExpr(out, n->condition, ccc);
        }
        {
            auto cc = ls; cc.push_back(true);
            appendIndent(out, cc);
            out << "body:\n";
            auto ccc = cc; ccc.push_back(true);
            printBody(out, n->body, ccc);
        }

    } else if (auto* n = dynamic_cast<const ForNode*>(node)) {
        out << "ForNode\n";
        if (n->iteratordef) {
            auto cc = ls; cc.push_back(false);
            appendIndent(out, cc);
            out << "init:\n";
            auto ccc = cc; ccc.push_back(true);
            printStatement(out, n->iteratordef, ccc);
        }
        if (n->assignement) {
            auto cc = ls; cc.push_back(false);
            appendIndent(out, cc);
            out << "step:\n";
            auto ccc = cc; ccc.push_back(true);
            printStatement(out, n->assignement, ccc);
        }
        if (n->loop) {
            auto cc = ls; cc.push_back(true);
            appendIndent(out, cc);
            out << "loop:\n";
            auto ccc = cc; ccc.push_back(true);
            printStatement(out, n->loop, ccc);
        }

    } else if (dynamic_cast<const ContinueNode*>(node)) {
        out << "ContinueNode\n";

    } else if (dynamic_cast<const BreakNode*>(node)) {
        out << "BreakNode\n";

    } else {
        out << "UnknownStatementNode\n";
    }
}

static void printParam(std::ostringstream& out, const VarDefNode* p, std::vector<bool> ls) {
    appendIndent(out, ls);
    if (printIfError(out, p)) return;
    out << modifierPrefix(p->specifiers)
        << "ParamNode \"" << p->name << "\" : "
        << vartypeToString(p->type) << "\n";
}

static void printMember(std::ostringstream& out, const MemberNode* node, std::vector<bool> ls) {
    appendIndent(out, ls);
    if (!node) { out << "(null)\n"; return; }
    if (printIfError(out, node)) return;

    if (auto* n = dynamic_cast<const FunctionNode*>(node)) {
        out << modifierPrefix(n->specifiers)
            << "FunctionNode \"" << n->name << "\" -> "
            << vartypeToString(n->returnType) << "\n";
        size_t bodyCount = n->body ? n->body->statements.size() : 0;
        size_t total = n->params.size() + bodyCount;
        size_t idx = 0;
        for (auto* p : n->params) {
            auto c = ls; c.push_back(++idx == total);
            printParam(out, p, c);
        }
        if (n->body) {
            for (auto* s : n->body->statements) {
                auto c = ls; c.push_back(++idx == total);
                printStatement(out, s, c);
            }
        }

    } else if (auto* n = dynamic_cast<const VarDefNode*>(node)) {
        out << modifierPrefix(n->specifiers)
            << "FieldNode \"" << n->name << "\" : "
            << vartypeToString(n->type) << "\n";
        if (n->value) { auto c = ls; c.push_back(true); printExpr(out, n->value, c); }

    } else if (auto* n = dynamic_cast<const StructNode*>(node)) {
        out << "StructNode \"" << n->name << "\"\n";
        for (int i = 0; i < (int)n->members.size(); i++) {
            auto c = ls; c.push_back(i == (int)n->members.size() - 1);
            printMember(out, n->members[i], c);
        }
    } else if (auto* n = dynamic_cast<const KernelDefinitionNode*>(node)) {
        out << "KernelDefinition \"" << n->name << "\" -> "
            << vartypeToString((PrimitiveType::VOID)) << "\n";
        size_t bodyCount = n->body ? n->body->statements.size() : 0;
        size_t total = n->params.size() + bodyCount;
        size_t idx = 0;
        for (auto* p : n->params) {
            auto c = ls; c.push_back(++idx == total);
            printParam(out, p, c);
        }
        if (n->body) {
            for (auto* s : n->body->statements) {
                auto c = ls; c.push_back(++idx == total);
                printStatement(out, s, c);
            }
        }

    } else {
        out << "UnknownMemberNode\n";
    }
}

static void printDecl(std::ostringstream& out, const DeclNode* node, std::vector<bool> ls) {
    appendIndent(out, ls);
    if (!node) { out << "(null)\n"; return; }
    if (printIfError(out, node)) return;

    if (auto* n = dynamic_cast<const StructNode*>(node)) {
        out << "StructNode \"" << n->name << "\"\n";
        for (int i = 0; i < (int)n->members.size(); i++) {
            auto c = ls; c.push_back(i == (int)n->members.size() - 1);
            printMember(out, n->members[i], c);
        }

    } else if (auto* n = dynamic_cast<const FunctionNode*>(node)) {
        out << modifierPrefix(n->specifiers)
            << "FunctionNode \"" << n->name << "\" -> "
            << vartypeToString(n->returnType) << "\n";
        size_t bodyCount = n->body ? n->body->statements.size() : 0;
        size_t total = n->params.size() + bodyCount;
        size_t idx = 0;
        for (auto* p : n->params) {
            auto c = ls; c.push_back(++idx == total);
            printParam(out, p, c);
        }
        if (n->body) {
            for (auto* s : n->body->statements) {
                auto c = ls; c.push_back(++idx == total);
                printStatement(out, s, c);
            }
        }

    } else if (auto* n = dynamic_cast<const VarDefNode*>(node)) {
        out << modifierPrefix(n->specifiers)
            << "VarDefNode \"" << n->name << "\" : "
            << vartypeToString(n->type) << "\n";
        if (n->value) { auto c = ls; c.push_back(true); printExpr(out, n->value, c); }

    } else if (auto* n = dynamic_cast<const NamespaceNode*>(node)) {
        out << "NamespaceNode \"" << n->name << "\"\n";
        for (int i = 0; i < (int)n->declarations.size(); i++) {
            auto c = ls; c.push_back(i == (int)n->declarations.size() - 1);
            printDecl(out, n->declarations[i], c);
        }
    } else if (auto* n = dynamic_cast<const KernelDefinitionNode*>(node)) {
        out << "KernelDefinitionNode \"" << n->name << "\" -> "
            << vartypeToString((PrimitiveType::VOID)) << "\n";
        size_t bodyCount = n->body ? n->body->statements.size() : 0;
        size_t total = n->params.size() + bodyCount;
        size_t idx = 0;
        for (auto* p : n->params) {
            auto c = ls; c.push_back(++idx == total);
            printParam(out, p, c);
        }
        if (n->body) {
            for (auto* s : n->body->statements) {
                auto c = ls; c.push_back(++idx == total);
                printStatement(out, s, c);
            }
        }

    } else {
        out << "UnknownDeclNode\n";
    }
}

static std::string toString(DeclNode* root) {
    std::ostringstream out;
    printDecl(out, root, {});
    return out.str();
}

static void print(DeclNode* root) {
    std::cout << toString(root);
}

static void writeToFile(DeclNode* root, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error("Could not open file for AST output: " + filepath);
    file << toString(root);
}

}