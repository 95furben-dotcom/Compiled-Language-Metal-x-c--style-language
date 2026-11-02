#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>
#include <sstream>


enum class PrimitiveType : uint8_t { UCHAR, CHAR, BOOL, SHORT, USHORT, INT, UINT, FLOAT, LONG, ULONG, DOUBLE, VOID};
static const std::vector<std::string> primitives = { "uchar","char", "bool", "short", "ushort", "int", "uint", "float", "long", "ulong", "double", "void"};

static PrimitiveType stringToPrimitiveType(std::string s){
    for(uint8_t i = 0; i < primitives.size(); i++){
        if(primitives[i] == s)
            return PrimitiveType(i);
    }

    throw std::runtime_error("invalid type " + s);
}

struct Vartype{
    std::optional<PrimitiveType> primitivetype;

    std::optional<std::vector<std::string>> structPath;
    Vartype* vectorType; // is 

    bool IsPrimitive(){
        return !structPath.has_value() && vectorType == nullptr;
    }

    Vartype(std::optional<PrimitiveType> primitivetype, std::optional<std::vector<std::string>> structPath, Vartype* vectorType)
        : primitivetype(primitivetype), structPath(structPath), vectorType(vectorType){}

    static Vartype NewPrimitive(PrimitiveType t){
        return Vartype(t,{}, nullptr);
    }

    static Vartype NewStructType(const std::vector<std::string>& structPath){
        return Vartype({},structPath, nullptr);
    }

    static Vartype NewVectorType(Vartype* vectorType){
        return Vartype({},{}, vectorType);
    }

    static bool IsStringPrimitive(const std::string& s){

        return std::find(primitives.begin(), primitives.end(), s) != primitives.end();
    }

    
};

static const Vartype STRINGTYPE = Vartype::NewVectorType(new Vartype(Vartype::NewPrimitive(PrimitiveType::CHAR)));

static std::string join(const std::vector<std::string>& v, std::string sep){
    std::ostringstream oss;
    for(int i = 0; i < (int)v.size(); i++){
        if(i > 0) oss << sep;
        oss << v[i];
    }
    return oss.str();
}
static std::string join(const std::vector<std::string>& v, char sep){
    std::ostringstream oss;
    for(int i = 0; i < (int)v.size(); i++){
        if(i > 0) oss << sep;
        oss << v[i];
    }
    return oss.str();
}


static std::string vartypeToString(Vartype t){
    if(t.IsPrimitive())
        return primitives[(uint)t.primitivetype.value()];
    if(t.structPath.has_value()) 
        return join(t.structPath.value(), ".");
    if(t.vectorType == nullptr) 
        throw std::runtime_error("Invalid Vartype: no value");
    return "vector<"+vartypeToString(*t.vectorType)+">";
    
}

