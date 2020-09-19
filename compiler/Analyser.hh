#pragma once

#include <Ast.hh>

struct Analyser : public AstVisitor {
    void visit(BinExpr *) override;
    void visit(NumLit *) override;
    void visit(RetStmt *) override;
};
