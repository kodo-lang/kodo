#include <Analyser.hh>
#include <Ast.hh>
#include <CharStream.hh>
#include <CodeGen.hh>
#include <IrGen.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <ir/Dumper.hh>

#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <memory>
#include <sstream>

constexpr const char *INPUT = R"(
fn main() -> i32 {
    return 5;
}
)";

int main() {
    std::istringstream istream(INPUT);
    CharStream stream(&istream);
    Lexer lexer(&stream);
    Parser parser(&lexer);
    auto *ast = parser.parse();
    auto program = gen_ir(ast);
    dump_ir(program.get());

    AstPrinter printer;
    printer.accept(ast);
    std::cout << '\n';

    Analyser analyser;
    analyser.accept(ast);

    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module(new llvm::Module("main", context));
    CodeGen code_gen(module.get());
    code_gen.accept(ast);
    auto *function = module->getFunction("main");
    module->print(llvm::errs(), nullptr);

    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::EngineBuilder engine_builder(std::move(module));
    engine_builder.setEngineKind(llvm::EngineKind::Either);
    auto *engine = engine_builder.create();
    llvm::errs() << engine->runFunctionAsMain(function, {"hello"}, nullptr) << '\n';
}
