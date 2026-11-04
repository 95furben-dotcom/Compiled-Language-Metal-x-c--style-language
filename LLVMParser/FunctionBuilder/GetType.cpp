#include "../Symbolscanner/SymbolScanner.hpp"
#include "../../Vartype.hpp"

#include "FunctionBuilder.hpp"
llvm::Type * LLVMParser::FunctionBuilder::GetLLVMType(Vartype t)  {
    if(t.IsPrimitive()){
        switch (t.primitivetype.value()) {
            case PrimitiveType::BOOL:   return llvm::Type::getInt1Ty(ctx);
            case PrimitiveType::CHAR:   return llvm::Type::getInt8Ty(ctx);
            case PrimitiveType::UCHAR:
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
    }
    if(t.structPath.has_value()){
        return symbolScanner.FindStruct(t.structPath.value(), path)->llvmType;
    }
}