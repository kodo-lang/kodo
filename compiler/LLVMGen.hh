#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <memory>

namespace ir {

class Program;

} // namespace ir

std::unique_ptr<llvm::Module> gen_llvm(const ir::Program *program, llvm::LLVMContext *llvm_context);
