#include <Analyser.hh>
#include <Ast.hh>
#include <CharStream.hh>
#include <CodeGen.hh>
#include <Lexer.hh>
#include <Parser.hh>

#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <memory>
#include <sstream>

constexpr const char *INPUT = R"(
fn main(argc: i32) -> i32 {
    return argc;
}
)";

int main() {
    std::istringstream istream(INPUT);
    CharStream stream(&istream);
    Lexer lexer(&stream);
    Parser parser(&lexer);
    auto *node = parser.parse();

    Analyser analyser;
    analyser.accept(node);

    AstPrinter printer;
    printer.accept(node);
    std::cout << '\n';

    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module(new llvm::Module("main", context));
    CodeGen code_gen(module.get());
    code_gen.accept(node);
    auto *function = module->getFunction("main");
    module->print(llvm::errs(), nullptr);

    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::EngineBuilder engine_builder(std::move(module));
    engine_builder.setEngineKind(llvm::EngineKind::Either);
    auto *engine = engine_builder.create();
    llvm::errs() << engine->runFunctionAsMain(function, {}, nullptr) << '\n';
}
