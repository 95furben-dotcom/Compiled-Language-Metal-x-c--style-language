#pragma once
#include <iostream>
#include <string>
#include <vector>

#include "SymbolEntries.hpp"   // wherever the entries live
#include "SymbolScanner.hpp"
#include "fstream"

namespace EntryPrinter {

inline std::string esc(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

inline std::string pad(int level) { return std::string(level * 2, ' '); }

inline std::string pathToJson(const Path& p) {
    std::string out = "[";
    for (size_t i = 0; i < p.size(); i++) {
        if (i) out += ", ";
        out += "\"" + esc(p[i]) + "\"";
    }
    return out + "]";
}

// Adjust to your Specifiers.hpp (add CONST, INLINE, ... as needed)
inline std::string specifiersToJson(uint spec) {
    std::vector<std::string> names;
    if (spec & Specifiers::STATIC) names.push_back("static");
    // if (spec & Specifiers::CONST)  names.push_back("const");
    // if (spec & Specifiers::INLINE) names.push_back("inline");

    std::string out = "[";
    for (size_t i = 0; i < names.size(); i++) {
        if (i) out += ", ";
        out += "\"" + names[i] + "\"";
    }
    return out + "]";
}

inline std::string typeToJson(const Vartype& t) {
    // uses your existing vartypeToString; if it was changed/removed in the
    // new branch, build the string from t.defaulttype / t.structPath here
    return "\"" + esc(vartypeToString(t)) + "\"";
}

// ---------- one overload per struct ----------

inline std::string toJson(const VariableEntry& e, int lvl = 0) {
    std::string in = pad(lvl + 1);
    std::string s = "{\n";
    s += in + "\"kind\": \"variable\",\n";
    s += in + "\"path\": " + pathToJson(e.path) + ",\n";
    s += in + "\"type\": " + typeToJson(e.type) + ",\n";
    s += in + "\"specifiers\": " + specifiersToJson(e.specifiers) + "\n";
    s += pad(lvl) + "}";
    return s;
}

inline std::string toJson(const FunctionEntry& e, int lvl = 0) {
    std::string in = pad(lvl + 1);
    std::string s = "{\n";
    s += in + "\"kind\": \"function\",\n";
    s += in + "\"path\": " + pathToJson(e.path) + ",\n";
    s += in + "\"returnType\": " + typeToJson(e.returnType) + ",\n";
    s += in + "\"specifiers\": " + specifiersToJson(e.specifiers) + ",\n";
    s += in + "\"parameters\": [";
    for (size_t i = 0; i < e.parameters.size(); i++) {
        if (i) s += ", ";
        s += "{\"name\": \"" + esc(e.parameters[i]->name) + "\", \"type\": "
           + typeToJson(e.parameters[i]->type) + "}";
    }
    s += "]\n" + pad(lvl) + "}";
    return s;
}

inline std::string toJson(const StructEntry& e, int lvl = 0) {
    std::string in = pad(lvl + 1);
    std::string s = "{\n";
    s += in + "\"kind\": \"struct\",\n";
    s += in + "\"path\": " + pathToJson(e.path) + ",\n";
    s += in + "\"size\": " + std::to_string(e.size) + ",\n";
    s += in + "\"fields\": [";
    for (size_t i = 0; i < e.fields.size(); i++) {
        s += (i ? ",\n" : "\n") + pad(lvl + 2) + toJson(e.fields[i], lvl + 2);
    }
    s += (e.fields.empty() ? "" : "\n" + in) + "],\n";
    s += in + "\"dependencies\": [";
    for (size_t i = 0; i < e.dependencies.size(); i++) {
        if (i) s += ", ";
        s += pathToJson(e.dependencies[i]);
    }
    s += "]\n" + pad(lvl) + "}";
    return s;
}

inline std::string toJson(const KernelEntry& e, int lvl = 0) {
    std::string in = pad(lvl + 1);
    std::string s = "{\n";
    s += in + "\"kind\": \"function\",\n";
    s += in + "\"path\": " + pathToJson(e.path) + ",\n";
    s += in + "\"parameters\": [";
    for (size_t i = 0; i < e.parameters.size(); i++) {
        if (i) s += ", ";
        s += "{\"name\": \"" + esc(e.parameters[i]->name) + "\", \"type\": "
           + typeToJson(e.parameters[i]->type) + "}";
    }
    s += "]\n" + pad(lvl) + "}";
    return s;
}

// ---------- dispatcher: works on any SymbolEntry ----------

inline std::string toJson(const SymbolEntry& e, int lvl = 0) {
    if (auto* s = dynamic_cast<const StructEntry*>(&e))   return toJson(*s, lvl);
    if (auto* f = dynamic_cast<const FunctionEntry*>(&e)) return toJson(*f, lvl);
    if (auto* v = dynamic_cast<const VariableEntry*>(&e)) return toJson(*v, lvl);
    if (auto* v = dynamic_cast<const KernelEntry*>(&e)) return toJson(*v, lvl);
    return "{\"kind\": \"unknown\", \"path\": " + pathToJson(e.path) + "}";
}

inline void print(const SymbolEntry& e) { std::cout << toJson(e) << "\n"; }

template <typename T>
inline void printAll(const std::vector<T>& entries) {
    for (const auto& e : entries) print(e);
}

inline std::string toString(const SymbolScanner& s) {
    std::ostringstream out;

    // summary header
    out << "SymbolScannerFound{\n";
    out << "\tfunctionEntries: "       << s.functionEntries.size()       << "\n";
    out << "\tstructEntries: "         << s.structEntries.size()         << "\n";
    out << "\tstaticVariableEntries: " << s.staticVariableEntries.size() << "\n";
    out << "\tkernelEntries: "         << s.kernelEntries.size()         << "\n";
    out << "}\n\n";

    // full entries
    auto section = [&](const char* name, const auto& entries) {
        out << name << ":\n";
        for (const auto& e : entries)
            out << EntryPrinter::toJson(e) << "\n";
        out << "\n";
    };

    section("structEntries",         s.structEntries);
    section("functionEntries",       s.functionEntries);
    section("staticVariableEntries", s.staticVariableEntries);
    section("kernelEntries",         s.kernelEntries);

    return out.str();
}

inline void print(const SymbolScanner& s) {
    std::cout << toString(s);
}

inline void writeToFile(const SymbolScanner& s, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error("Could not open file for SymbolScanner output: " + filepath);
    file << toString(s);
}
} 

