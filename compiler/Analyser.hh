#pragma once

#include <Ast.hh>

struct Analyser : public AstVisitor {
    void visit(AssignStmt *) override;
    void visit(BinExpr *) override;
    void visit(DeclStmt *) override;
    void visit(FunctionDecl *) override;
    void visit(NumLit *) override;
    void visit(RetStmt *) override;
    void visit(VarExpr *) override;
};
