#pragma once

namespace ast {

class AssignExpr;
class BinExpr;
class CallExpr;
class DeclStmt;
class FunctionArg;
class FunctionDecl;
class NumLit;
class RetStmt;
class Root;
class UnaryExpr;
class VarExpr;

struct Visitor {
    virtual void visit(const AssignExpr *) = 0;
    virtual void visit(const BinExpr *) = 0;
    virtual void visit(const CallExpr *) = 0;
    virtual void visit(const DeclStmt *) = 0;
    virtual void visit(const FunctionArg *) = 0;
    virtual void visit(const FunctionDecl *) = 0;
    virtual void visit(const NumLit *) = 0;
    virtual void visit(const RetStmt *) = 0;
    virtual void visit(const Root *) = 0;
    virtual void visit(const UnaryExpr *) = 0;
    virtual void visit(const VarExpr *) = 0;
};

} // namespace ast
