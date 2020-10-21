#include <CharStream.hh>
#include <IrGen.hh>
#include <LLVMGen.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <TypeChecker.hh>
#include <VarChecker.hh>
#include <ast/Dumper.hh>
#include <ir/Dumper.hh>
#include <pass/PassManager.hh>
#include <support/ArgsParser.hh>
#include <support/Error.hh>

#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

int main(int argc, char **argv) {
    args::Parser args_parser;
    args::Value<bool> dump_ast_opt(false);
    args::Value<bool> dump_ir_opt(false);
    args::Value<bool> dump_llvm_opt(false);
    args::Value<bool> verify_llvm_opt(true);
    std::string mode_string;
    std::string input_file;
    args_parser.add_arg(&mode_string);
    args_parser.add_arg(&input_file);
    args_parser.add_option("dump-ast", &dump_ast_opt);
    args_parser.add_option("dump-ir", &dump_ir_opt);
    args_parser.add_option("dump-llvm", &dump_llvm_opt);
    args_parser.add_option("verify-llvm", &verify_llvm_opt);
    args_parser.parse(argc, argv);

    bool run = mode_string == "run";
    if (mode_string != "build" && mode_string != "run") {
        throw std::runtime_error("Invalid mode " + mode_string);
    }

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
    PassManager pass_manager;
    pass_manager.add<TypeChecker>();
    pass_manager.add<VarChecker>();
    if (dump_ir_opt.present_or_true()) {
        pass_manager.add<ir::Dumper>();
    }
    pass_manager.run(*program);
    abort_if_error();

    llvm::LLVMContext context;
    auto module = gen_llvm(program.get(), &context);
    if (verify_llvm_opt.present_or_true() && llvm::verifyModule(*module, &llvm::errs())) {
        llvm::errs() << '\n';
        return 1;
    }

    bool dump_llvm = dump_llvm_opt.present_or_true();
    dump_llvm |= !run;
    if (dump_llvm) {
        module->print(llvm::errs(), nullptr);
    }
    if (!run) {
        return 0;
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
