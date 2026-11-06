
#include "../Vartype.hpp"

#include "LLVMBuilder.hpp"

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
    throw CodeGenError("Vectors not implimented");
}