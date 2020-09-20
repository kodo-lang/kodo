#pragma once

#include <Ast.hh>

#include <unordered_map>

class Analyser : public AstVisitor {
    std::unordered_map<std::string_view, Type *> m_vars;

public:
    void visit(AssignStmt *) override;
    void visit(BinExpr *) override;
    void visit(DeclStmt *) override;
    void visit(FunctionArg *) override;
    void visit(FunctionDecl *) override;
    void visit(NumLit *) override;
    void visit(RetStmt *) override;
    void visit(UnaryExpr *) override;
    void visit(VarExpr *) override;
};
