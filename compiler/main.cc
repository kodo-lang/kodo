#include <CharStream.hh>
#include <IrGen.hh>
#include <LLVMGen.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <TypeChecker.hh>
#include <ast/Dumper.hh>
#include <ir/Dumper.hh>

#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <memory>
#include <sstream>

constexpr const char *INPUT = R"(
// extern some libc functions.
extern fn malloc(size: i64) => *i32;
extern fn putchar(character: i32) => i32;

fn main() => i32 {
    var h: *i32 = malloc(1);
    var i: *i32 = malloc(1);
    var newline: *i32 = malloc(1);
    *h = 72;
    *i = 105;
    *newline = 10;
    putchar(*h);
    putchar(*i);
    putchar(*newline);

    // Implicit int extension cast.
    var foo: i16 = 5;
    var bar: i32 = foo + 5;
    return bar;
}
)";

int main() {
    std::istringstream istream(INPUT);
    CharStream stream(&istream);
    Lexer lexer(&stream);
    Parser parser(&lexer);

    auto ast = parser.parse();
    ast::dump(ast.get());
    std::cout << '\n';

    auto program = gen_ir(ast.get());
    type_check(program.get());
    dump_ir(program.get());
    std::cout << '\n';

    llvm::LLVMContext context;
    auto module = gen_llvm(program.get(), &context);
    module->print(llvm::errs(), nullptr);
    if (llvm::verifyModule(*module, &llvm::errs())) {
        llvm::errs() << '\n';
        return 1;
    }

    auto *function = module->getFunction("main");
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::EngineBuilder engine_builder(std::move(module));
    engine_builder.setEngineKind(llvm::EngineKind::Either);
    std::unique_ptr<llvm::ExecutionEngine> engine(engine_builder.create());
    llvm::errs() << engine->runFunctionAsMain(function, {"hello"}, nullptr) << '\n';
}
