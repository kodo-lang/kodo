#include <Ast.hh>
#include <CharStream.hh>
#include <Lexer.hh>
#include <Parser.hh>

#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <sstream>

constexpr const char *INPUT = R"(
5 + 7
)";

int main() {
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module(new llvm::Module("main", context));
    auto *function = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getInt32Ty(context), false),
                                            llvm::Function::ExternalLinkage, "main", *module);
    auto *entry = llvm::BasicBlock::Create(context, "entry", function);
    llvm::IRBuilder<> builder(context);
    builder.SetInsertPoint(entry);
    builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 5));
    module->print(llvm::errs(), nullptr);

    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::EngineBuilder engine_builder(std::move(module));
    engine_builder.setEngineKind(llvm::EngineKind::Either);
    auto *engine = engine_builder.create();
    llvm::errs() << engine->runFunctionAsMain(function, {}, nullptr) << '\n';

    std::istringstream istream(INPUT);
    CharStream stream(&istream);
    Lexer lexer(&stream);
    Parser parser(&lexer);
    AstPrinter printer;
    printer.accept(parser.parse());
    std::cout << '\n';
}
