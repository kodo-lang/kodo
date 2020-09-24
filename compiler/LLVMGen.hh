#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <memory>

class Program;

std::unique_ptr<llvm::Module> gen_llvm(const Program *program, llvm::LLVMContext *llvm_context);
