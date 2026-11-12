#include "LLVMBuilder.hpp"
#include "../Vartype.hpp"
#include "Utillity/Caster.hpp"
#include "Utillity/LLVMDebug.hpp"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include <cstdint>
#include <cstdio>
#include <sys/types.h>

constexpr uint64_t STR_CONST_FLAG = 1ull << 63;
// real capacity = cap & ~STR_CONST_FLAG;  isConst = cap & STR_CONST_FLAG

static bool isFloat(const Vartype& t) {
    return t.primitivetype &&
           (t.primitivetype.value() == PrimitiveType::FLOAT ||
            t.primitivetype.value() == PrimitiveType::DOUBLE);
}

// bigger rank = "wider" type
static int rank(PrimitiveType p) {
    switch (p) {
        case PrimitiveType::BOOL:   return 0;
        case PrimitiveType::CHAR:
        case PrimitiveType::UCHAR:  return 1;
        case PrimitiveType::SHORT:
        case PrimitiveType::USHORT: return 2;
        case PrimitiveType::INT:
        case PrimitiveType::UINT:   return 3;
        case PrimitiveType::LONG:
        case PrimitiveType::ULONG:  return 4;
        case PrimitiveType::FLOAT:  return 5;
        case PrimitiveType::DOUBLE: return 6;
        default: throw CodeGenError("rank: not a numeric type");
    }
}

// the type both operands are converted to before an operator
static Vartype commonType(const Vartype& a, const Vartype& b) {
    if (!a.primitivetype || !b.primitivetype)
        throw CodeGenError("Operators on non-primitive types not implemented");
    int ra = rank(a.primitivetype.value()), rb = rank(b.primitivetype.value());
    if (ra != rb) return ra > rb ? a : b;
    // same width: prefer unsigned, like C
    return isPrimitiveSigned(a.primitivetype.value()) ? b : a;
}

bool LLVMBuilder::isSigned(const Vartype& a) {
    return a.primitivetype ? isPrimitiveSigned(a.primitivetype.value()) : false;
}

// ---------- conversion: the one place sext/zext/sitofp live ----------

LLVMBuilder::TypedValue LLVMBuilder::CastTo(TypedValue v, const Vartype& target, const Path& path) {
    llvm::Type* to   = GetLLVMType(target, path);
    llvm::Type* from = v.value->getType();
    if (from == to) return {v.value, target};

    bool fromFP = from->isFloatingPointTy();
    bool toFP   = to->isFloatingPointTy();
    llvm::Value* r;

    if (!fromFP && !toFP)      r = builder.CreateIntCast(v.value, to, isSigned(v.type)); // sext/zext/trunc
    else if (!fromFP && toFP)  r = isSigned(v.type) ? builder.CreateSIToFP(v.value, to)
                                                    : builder.CreateUIToFP(v.value, to);
    else if (fromFP && !toFP)  r = isSigned(target) ? builder.CreateFPToSI(v.value, to)
                                                    : builder.CreateFPToUI(v.value, to);
    else                       r = builder.CreateFPCast(v.value, to);                    // fpext/fptrunc
    return {r, target};
}

// ---------- operators ----------

LLVMBuilder::TypedValue LLVMBuilder::BuildOprand(TypedValue a, TypedValue b,
                                                 const std::string& op, const Path& path) {
    Vartype t = commonType(a.type, b.type);
    llvm::Value* x = CastTo(a, t, path).value;
    llvm::Value* y = CastTo(b, t, path).value;
    bool fp = isFloat(t);
    bool sg = isSigned(t);

    if (op == "+") return {fp ? builder.CreateFAdd(x, y, "sum")  : builder.CreateAdd(x, y, "sum"),  t};
    if (op == "-") return {fp ? builder.CreateFSub(x, y, "diff") : builder.CreateSub(x, y, "diff"), t};
    if (op == "*") return {fp ? builder.CreateFMul(x, y, "prod") : builder.CreateMul(x, y, "prod"), t};
    if (op == "%") return {fp ? builder.CreateFRem(x, y,"rem")   : (sg ? builder.CreateSRem(x,y,"rem") : builder.CreateURem(x,y,"rem")),   t};
    if (op == "==") return {builder.CreateICmpEQ(x, y,"result"),   PrimitiveType::BOOL};
    if (op == "!=") return {builder.CreateICmpNE(x, y,"result"),   PrimitiveType::BOOL};
    if (op == ">=") return {sg ? builder.CreateICmpSGE(x, y,"result") :builder.CreateICmpUGE(x, y,"result") ,   PrimitiveType::BOOL};
    if (op == "<=") return {sg ? builder.CreateICmpSLE(x, y,"result") :builder.CreateICmpULE(x, y,"result") ,   PrimitiveType::BOOL};
    if (op == ">") return {sg ? builder.CreateICmpSGT(x, y,"result") :builder.CreateICmpUGT(x, y,"result") ,   PrimitiveType::BOOL};
    if (op == "<") return {sg ? builder.CreateICmpSLT(x, y,"result") :builder.CreateICmpULT(x, y,"result") ,   PrimitiveType::BOOL};
    if (op == "/") {
        if (fp) return {builder.CreateFDiv(x, y, "quot"), t};
        return {sg ? builder.CreateSDiv(x, y, "quot") : builder.CreateUDiv(x, y, "quot"), t};
    }
    throw CodeGenError("Unknown operator: " + op);
}

// ---------- values ----------

LLVMBuilder::TypedValue LLVMBuilder::BuildValue(const ExprNode* node, const Path& path) {
    if (auto* n = TryCast<NumberNode>(node)) {
        llvm::Type* ty = GetLLVMType(n->type, path);
        if(n->type == PrimitiveType::BOOL){
            if(n->value=="true")
                return {llvm::ConstantInt::getBool(ty, true),n->type};
            else 
                return {llvm::ConstantInt::getBool(ty, false),n->type};
        }
        printf("Parsing value %s\n",n->value.c_str());
        llvm::Constant* c;
        if (ty->isFloatTy())
            c = llvm::ConstantFP::get(ty, (double)std::stof(n->value));   // round to float first
        else if (ty->isDoubleTy())
            c = llvm::ConstantFP::get(ty, std::stod(n->value));
        else
            c = llvm::ConstantInt::get(ty, std::stoll(n->value), isSigned(n->type));
        return {c, n->type};
    }
    if (auto* n = TryCast<OprandNode>(node)) {
        TypedValue a = BuildValue(n->v1, path);
        TypedValue b = BuildValue(n->v2, path);
        return BuildOprand(a, b, n->oprand, path);
    }
    if (auto* n = TryCast<VariableCallNode>(node)) {
        // adjust to what FindStaticVar really returns
        printf("Finding var %s in path: %s\n",n->name.c_str(), join(path,'.').c_str());
        if (auto* v = symbolScanner.FindStaticVar({n->name}, path)) {
            GlobalVar& g = globals.at(join(v->path, '.'));
            return {builder.CreateLoad(g.llvmType, g.global), g.type};
        }
        throw CodeGenError("Unknown variable: " + n->name);   // locals come later
    }
    if(auto n = TryCast<StringNode>(node)){
        llvm::Value *v = builder.CreateGlobalString(n->value);
        llvm::Value *size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), n->value.size());
        llvm::Value *capasity = llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), STR_CONST_FLAG | (uint64_t)n->value.size());
        
        llvm::Type* ty =GetLLVMType(STRINGTYPE, path);
        llvm::AllocaInst *alloca = builder.CreateAlloca(ty, nullptr, "stringAlloca");
        TypedValue self{alloca, STRINGTYPE};
        TypedValue f0 = GetStructField(self, 0, path);
        TypedValue f1 = GetStructField(self, 1, path);
        TypedValue f2 = GetStructField(self, 2, path);
        builder.CreateStore(v,        f0.value);
        builder.CreateStore(size,     f1.value);
        builder.CreateStore(capasity, f2.value);

        
        return {builder.CreateLoad(ty, alloca), STRINGTYPE};
    }
    throw CodeGenError("BuildValue: expression type not implemented yet");
}

// ---------- constants = BuildValue with nothing to insert into ----------

llvm::Constant* LLVMBuilder::BuildConstant(const ExprNode* node, const Vartype& target, const Path& path) {
    builder.ClearInsertionPoint();                    // no block => only folded constants can result
    TypedValue v = CastTo(BuildValue(node, path), target, path);
    auto* c = llvm::dyn_cast<llvm::Constant>(v.value);
    if (!c) throw CodeGenError("static initializer must be a compile-time constant");
    return c;
}

LLVMBuilder::TypedValue LLVMBuilder::GetStructField(TypedValue s, int fieldIdx, Path path){
    if(s.type.vectorType){
        llvm::Value* fieldPtr = builder.CreateInBoundsGEP(
            GetLLVMType(s.type,path), 
            s.value,
            {builder.getInt32(0),builder.getInt32(fieldIdx)}
        );

        switch (fieldIdx) {
            case 0:
                return {fieldPtr, Vartype::NewPtr(PrimitiveType::CHAR)};
            case 1:
            case 2:
                return {fieldPtr, PrimitiveType::ULONG};
            default: throw CodeGenError("fieldIdx to big for vector");
        }

    }
    if(!s.type.structPath)  throw CodeGenError("Can not get struct field on nonstruct");

    auto e = symbolScanner.FindStruct(s.type.structPath.value(), path);
    if(!e) throw CodeGenError("Could not get symbol for struct: "+join(s.type.structPath.value(),'.'));
    if(e->fields.size() <= fieldIdx) throw CodeGenError("struct fiedl size smaller than field index");

    Vartype vt = e->fields[fieldIdx].type;
    llvm::Type* ty = GetLLVMType(vt, path);

    llvm::Value* fieldPtr = builder.CreateInBoundsGEP(
        GetLLVMType(s.type,path), 
        s.value,
        {builder.getInt32(0),builder.getInt32(fieldIdx)}
    );
    return {fieldPtr, vt};
}