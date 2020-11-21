#pragma once

namespace ast {

class AsmExpr;
class AssignExpr;
class BinExpr;
class Block;
class CallExpr;
class CastExpr;
class ConstructExpr;
class DeclStmt;
class FunctionArg;
class FunctionDecl;
class IfStmt;
class ImportStmt;
class MemberExpr;
class NumLit;
class PointerType;
class RetStmt;
class Root;
class StringLit;
class StructField;
class StructType;
class Symbol;
class TypeDecl;
class UnaryExpr;

struct Visitor {
    virtual void visit(const AsmExpr *) = 0;
    virtual void visit(const AssignExpr *) = 0;
    virtual void visit(const BinExpr *) = 0;
    virtual void visit(const Block *) = 0;
    virtual void visit(const CallExpr *) = 0;
    virtual void visit(const CastExpr *) = 0;
    virtual void visit(const ConstructExpr *) = 0;
    virtual void visit(const DeclStmt *) = 0;
    virtual void visit(const FunctionArg *) = 0;
    virtual void visit(const FunctionDecl *) = 0;
    virtual void visit(const IfStmt *) = 0;
    virtual void visit(const ImportStmt *) = 0;
    virtual void visit(const MemberExpr *) = 0;
    virtual void visit(const NumLit *) = 0;
    virtual void visit(const PointerType *) = 0;
    virtual void visit(const RetStmt *) = 0;
    virtual void visit(const Root *) = 0;
    virtual void visit(const StringLit *) = 0;
    virtual void visit(const StructField *) = 0;
    virtual void visit(const StructType *) = 0;
    virtual void visit(const Symbol *) = 0;
    virtual void visit(const TypeDecl *) = 0;
    virtual void visit(const UnaryExpr *) = 0;
};

} // namespace ast
