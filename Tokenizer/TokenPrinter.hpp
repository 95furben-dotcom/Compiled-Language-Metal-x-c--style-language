#pragma once
#include "Tokens.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <memory>

namespace TokenPrinter{
static std::string printToken(const Token& t) {
    std::ostringstream out;
    if (auto* p = dynamic_cast<const signToken*>(&t))
        out << "[sign] '" << p->c << "'\n";
    else if (auto* p = dynamic_cast<const typeToken*>(&t))
        out << "[type] " << vartypeToString(p->type) << "\n";
    else if (auto* p = dynamic_cast<const operatorToken*>(&t))
        out << "[operator] \"" << p->s << "\"\n";
    else if (auto* p = dynamic_cast<const assignmentToken*>(&t))
        out << "[assignment] '" << (p->c == '=' ? "" : "=")<<p->c << "'\n";
    else if (auto* p = dynamic_cast<const stringToken*>(&t))
        out << "[string] \"" << p->s << "\"\n";
    else if (auto* p = dynamic_cast<const endlToken*>(&t))
        out << "[endl]\n";
    else if (auto* p = dynamic_cast<const returnToken*>(&t))
        out << "[return]\n";
    else if (auto* p = dynamic_cast<const valueToken*>(&t))
        out << "[value] \"" << p->s << "\"\n";
    else if (auto* p = dynamic_cast<const definisionToken*>(&t))
        out << "[definision] \"" << p->name << "\"\n";
    else if (auto* p = dynamic_cast<const structToken*>(&t))
        out << "[struct]\n";
    else if (auto* p = dynamic_cast<const namespaceToken*>(&t))
        out << "[namespace]\n";
    else if (auto* p = dynamic_cast<const specifierToken*>(&t))
        out << "[specifier] \""<<p->s<<"\"\n";
    else if (auto* p = dynamic_cast<const iterateToken*>(&t))
        out << "["<<(p->isPluss ? "++" : "--")<<"]\n";
    else if (auto* p = dynamic_cast<const ifToken*>(&t))
        out << "[if]\n";
    else if (auto* p = dynamic_cast<const elseToken*>(&t))
        out << "[else]\n";
    else if (auto* p = dynamic_cast<const whileToken*>(&t))
        out << "[while]\n";
    else if (auto* p = dynamic_cast<const forToken*>(&t))
        out << "[for]\n";
    else if (auto* p = dynamic_cast<const continueToken*>(&t))
        out << "[continue]\n";
    else if (auto* p = dynamic_cast<const breakToken*>(&t))
        out << "[break]\n";

    else
        out << "[unknown]\n";
    return out.str();
}

static void printTokens(const std::vector<std::unique_ptr<Token>>& tokens) {
    for (size_t i = 0; i < tokens.size(); i++) {
        std::cout << i << ": ";
        std::cout << printToken(*tokens[i]);
    }
}

static void tokensToFile(const std::vector<std::unique_ptr<Token>>& tokens, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error("Could not open file at " + filepath);
    for (size_t i = 0; i < tokens.size(); i++) {
        file << i << ": ";
        file << printToken(*tokens[i]);
    }
}
}