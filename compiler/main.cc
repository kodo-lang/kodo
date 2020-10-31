#include <CharStream.hh>
#include <IrGen.hh>
#include <LLVMGen.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <TypeChecker.hh>
#include <ast/Dumper.hh>
#include <ir/Dumper.hh>
#include <support/ArgsParser.hh>

#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char **argv) {
    args::Parser args_parser;
    args::Value<bool> dump_ast_opt(false);
    args::Value<bool> dump_ir_opt(false);
    args::Value<bool> dump_llvm_opt(false);
    args::Value<bool> verify_llvm_opt(true);
    std::string input_file;
    args_parser.add_arg(&input_file);
    args_parser.add_option("dump-ast", &dump_ast_opt);
    args_parser.add_option("dump-ir", &dump_ir_opt);
    args_parser.add_option("dump-llvm", &dump_llvm_opt);
    args_parser.add_option("verify-llvm", &verify_llvm_opt);
    args_parser.parse(argc, argv);

    std::ifstream ifstream(input_file);
    CharStream stream(&ifstream);
    Lexer lexer(&stream);
    Parser parser(&lexer);

    auto ast = parser.parse();
    if (dump_ast_opt.present_or_true()) {
        ast::dump(ast.get());
        std::cout << '\n';
    }

    auto program = gen_ir(ast.get());
    type_check(program.get());
    if (dump_ir_opt.present_or_true()) {
        dump_ir(program.get());
        std::cout << '\n';
    }

    llvm::LLVMContext context;
    auto module = gen_llvm(program.get(), &context);
    if (dump_llvm_opt.present_or_true()) {
        module->print(llvm::errs(), nullptr);
    }
    if (verify_llvm_opt.present_or_true() && llvm::verifyModule(*module, &llvm::errs())) {
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
