#include <ast/Nodes.hh>

#include <ast/Visitor.hh>

namespace ast {

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

void NumLit::accept(Visitor *visitor) const {
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

void Symbol::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void UnaryExpr::accept(Visitor *visitor) const {
    visitor->visit(this);
}

} // namespace ast
