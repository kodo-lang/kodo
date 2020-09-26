#include <Ast.hh>
#include <CharStream.hh>
#include <IrGen.hh>
#include <LLVMGen.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <ir/Dumper.hh>

#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <sstream>

constexpr const char *INPUT = R"(
fn main() -> i32 {
    var foo: i32 = 5;
    var bar: *i32 = &foo;
    var baz: **i32 = &bar;
    return foo + *bar + **baz;
}
)";

int main() {
    std::istringstream istream(INPUT);
    CharStream stream(&istream);
    Lexer lexer(&stream);
    Parser parser(&lexer);

    auto *ast = parser.parse();
    AstPrinter printer;
    printer.accept(ast);
    std::cout << '\n';

    auto program = gen_ir(ast);
    dump_ir(program.get());

    llvm::LLVMContext context;
    auto module = gen_llvm(program.get(), &context);
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
