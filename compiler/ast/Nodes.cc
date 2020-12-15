#include <ast/Nodes.hh>

#include <ast/Visitor.hh>

namespace ast {

void AsmExpr::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void AssignExpr::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void BinExpr::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void Block::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void CallExpr::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void CastExpr::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void ConstDecl::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void ConstructExpr::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void DeclStmt::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void FunctionArg::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void FunctionDecl::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void IfStmt::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void ImportStmt::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void MemberExpr::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void NumLit::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void PointerType::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void RetStmt::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void Root::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void StringLit::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void StructField::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void StructType::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void Symbol::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void TraitType::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void TypeDecl::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void UnaryExpr::accept(Visitor *visitor) const {
    visitor->visit(this);
}

} // namespace ast
