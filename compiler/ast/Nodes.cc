#include <ast/Nodes.hh>

#include <ast/Visitor.hh>

namespace ast {

void AssignStmt::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void BinExpr::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void CallExpr::accept(Visitor *visitor) const {
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

void NumLit::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void RetStmt::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void Root::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void UnaryExpr::accept(Visitor *visitor) const {
    visitor->visit(this);
}

void VarExpr::accept(Visitor *visitor) const {
    visitor->visit(this);
}

} // namespace ast
