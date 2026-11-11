
#include "../Vartype.hpp"

#include "LLVMBuilder.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

llvm::Type * LLVMBuilder::GetLLVMType(PrimitiveType t)  {
    switch (t) {
        case PrimitiveType::BOOL:   return llvm::Type::getInt1Ty(ctx);
        case PrimitiveType::UCHAR: 
        case PrimitiveType::CHAR:   return llvm::Type::getInt8Ty(ctx);
        case PrimitiveType::SHORT:
        case PrimitiveType::USHORT: return llvm::Type::getInt16Ty(ctx);
        case PrimitiveType::INT:
        case PrimitiveType::UINT:   return llvm::Type::getInt32Ty(ctx);
        case PrimitiveType::FLOAT:  return llvm::Type::getFloatTy(ctx);
        case PrimitiveType::LONG:
        case PrimitiveType::ULONG:  return llvm::Type::getInt64Ty(ctx);
        case PrimitiveType::DOUBLE: return llvm::Type::getDoubleTy(ctx);

        case PrimitiveType::VOID: return llvm::Type::getVoidTy(ctx);
    }
    throw CodeGenError("Unhandled primitive type");

}

llvm::Type * LLVMBuilder::GetLLVMType(const Vartype& t, const Path& path)  {
    if(t.IsPrimitive()){
        return GetLLVMType(t.primitivetype.value());
    }
    if(t.structPath.has_value()){
        auto entry = symbolScanner.FindStruct(t.structPath.value(), path);
        if (!entry)
            throw CodeGenError("Unknown struct type: " + join(t.structPath.value(), '.'));
        if (!entry->llvmType)
            throw CodeGenError("Struct type not created yet: " + join(entry->path, '.'));
        return entry->llvmType;
    }
    if(t.IsPointer()){
        return llvm::PointerType::getUnqual(ctx);
    }
    //vector 
    return GetVectorType(t, path);
}


llvm::Type* LLVMBuilder::GetStructType(const Path& structPath, const Path& scope) {
    auto* e = symbolScanner.FindStruct(structPath, scope);
    if (!e) throw CodeGenError("Unknown struct: " + join(structPath, '.'));
    std::string key = join(e->path, '.');
    auto it = structTypes.find(key);
    if (it != structTypes.end()) return it->second;
    return structTypes[key] = llvm::StructType::create(ctx, key);   // opaque first
}

llvm::StructType* LLVMBuilder::GetVectorType(const Vartype& t, const Path& path) {
    // key must include the element type, so vector<int> != vector<char>
    std::string key = "vector<" + vartypeToString(*t.vectorType) + ">";   // adapt to your field
    auto it = structTypes.find(key);
    if (it != structTypes.end()) return it->second;

    llvm::Type* ptrTy = llvm::PointerType::getUnqual(ctx);
    llvm::Type* i64   = llvm::Type::getInt64Ty(ctx);
    auto* st = llvm::StructType::create(ctx, {ptrTy, i64, i64}, key);
    structTypes[key] = st;
    return st;
}