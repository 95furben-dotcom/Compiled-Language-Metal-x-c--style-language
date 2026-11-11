#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>
#include <sstream>
#include <memory>
using Path = std::vector<std::string>;
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

    std::optional<Path> structPath;
    Vartype* vectorType; // 8 byte pointer to data 8 byte number for size 8 byte number for capasity 

    std::shared_ptr<Vartype> pointee;     // non-null => this is a pointer to *pointee
    bool IsPointer() const { return pointee != nullptr; }

    bool IsPrimitive() const{
        return primitivetype.has_value();
    }

    Vartype(std::optional<PrimitiveType> primitivetype, std::optional<std::vector<std::string>> structPath, Vartype* vectorType,std::shared_ptr<Vartype> pointee = nullptr)
        : primitivetype(primitivetype), structPath(structPath), vectorType(vectorType), pointee(pointee){}

    Vartype(PrimitiveType t) : primitivetype(t),structPath({}),vectorType(nullptr){}

    static Vartype NewStructType(const std::vector<std::string>& structPath){
        return Vartype({},structPath, nullptr);
    }

    static Vartype NewVectorType(Vartype* vectorType){
        return Vartype({},{}, vectorType);
    }

    static bool IsStringPrimitive(const std::string& s){

        return std::find(primitives.begin(), primitives.end(), s) != primitives.end();
    }

    static Vartype NewPtr(Vartype ptrTy){
        return Vartype({},{}, nullptr, std::make_shared<Vartype>(ptrTy));
    }
    
    bool IsString(){
        return (vectorType && vectorType->IsPrimitive() && vectorType->primitivetype.value() == PrimitiveType::CHAR);
    }
};

static const Vartype STRINGTYPE = Vartype::NewVectorType(new Vartype(PrimitiveType::CHAR));

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
    if(t.vectorType) 
        return "vector<"+vartypeToString(*t.vectorType)+">";
    if(t.pointee)
        return "ptr<" + vartypeToString(*t.pointee);
    throw std::runtime_error("Invalid Vartype: no value");
    
}


static bool isPrimitiveSigned(PrimitiveType t){
    switch (t) {
    case PrimitiveType::UCHAR:
    case PrimitiveType::USHORT:
    case PrimitiveType::ULONG:
    case PrimitiveType::UINT:
    case PrimitiveType::BOOL:
    case PrimitiveType::VOID:
        return false;
    case PrimitiveType::CHAR:
    case PrimitiveType::SHORT:
    case PrimitiveType::INT:
    case PrimitiveType::FLOAT:
    case PrimitiveType::LONG:
    case PrimitiveType::DOUBLE:
        return true;
    }
}

static uint PrimitiveSize(PrimitiveType v){
    switch (v) {
        case PrimitiveType::BOOL:
        case PrimitiveType::CHAR:
        return 1;
        case PrimitiveType::SHORT:
        case PrimitiveType::USHORT:
        return 2;

        case PrimitiveType::INT:
        case PrimitiveType::UINT:
        case PrimitiveType::FLOAT:
        return 4;

        case PrimitiveType::LONG:
        case PrimitiveType::ULONG:
        case PrimitiveType::DOUBLE:
        return 8;

        case PrimitiveType::VOID:
            throw std::runtime_error("Can not get primitive size of VOID");
        default:
        throw std::runtime_error("Invalid type vartype");
    }
}