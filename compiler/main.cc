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

#include <fstream>
#include <iostream>
#include <memory>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <source>\n";
        exit(1);
    }

    bool silent = argc == 3 && std::string(argv[1]) == "--silent";
    std::ifstream ifstream(argv[argc - 1]);
    CharStream stream(&ifstream);
    Lexer lexer(&stream);
    Parser parser(&lexer);

    auto ast = parser.parse();
    if (!silent) {
        ast::dump(ast.get());
        std::cout << '\n';
    }

    auto program = gen_ir(ast.get());
    type_check(program.get());
    if (!silent) {
        dump_ir(program.get());
        std::cout << '\n';
    }

    llvm::LLVMContext context;
    auto module = gen_llvm(program.get(), &context);
    if (!silent) {
        module->print(llvm::errs(), nullptr);
    }
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
    return engine->runFunctionAsMain(function, {"hello"}, nullptr);
}
