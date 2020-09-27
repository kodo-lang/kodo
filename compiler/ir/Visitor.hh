#pragma once

class BinaryInst;
class BranchInst;
class CallInst;
class CompareInst;
class CondBranchInst;
class LoadInst;
class PhiInst;
class StoreInst;
class RetInst;

struct Visitor {
    virtual void visit(BinaryInst *) = 0;
    virtual void visit(BranchInst *) = 0;
    virtual void visit(CallInst *) = 0;
    virtual void visit(CompareInst *) = 0;
    virtual void visit(CondBranchInst *) = 0;
    virtual void visit(LoadInst *) = 0;
    virtual void visit(PhiInst *) = 0;
    virtual void visit(StoreInst *) = 0;
    virtual void visit(RetInst *) = 0;
};
