#include "LLVMBuilder.hpp"
#include "../Vartype.hpp"

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
        llvm::Constant* c = ty->isFloatingPointTy()
            ? llvm::ConstantFP::get(ty, std::stod(n->value))
            : llvm::ConstantInt::get(ty, std::stoll(n->value), isSigned(n->type));
        return {c, n->type};
    }
    if (auto* n = TryCast<OprandNode>(node)) {
        TypedValue a = BuildValue(n->v1, path);
        TypedValue b = BuildValue(n->v2, path);
        return BuildOprand(a, b, n->oprand, path);
    }
    if (auto* n = TryCast<VariableCallNode>(node)) {
        // adjust to what FindStaticVar really returns
        if (auto* v = symbolScanner.FindStaticVar(path, {n->name})) {
            GlobalVar& g = globals.at(join(v->path, '.'));
            return {builder.CreateLoad(g.llvmType, g.global), g.type};
        }
        throw CodeGenError("Unknown variable: " + n->name);   // locals come later
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