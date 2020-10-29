#include <Compiler.hh>
#include <LLVMGen.hh>
#include <TypeChecker.hh>
#include <VarChecker.hh>
#include <ir/Dumper.hh>
#include <pass/PassManager.hh>
#include <support/ArgsParser.hh>
#include <support/Error.hh>

#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

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

    Compiler compiler;
    auto program = compiler.compile(input_file);

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
    if (verify_llvm_opt.present_or_true()) {
        auto print_newline = [] {
            llvm::errs() << '\n';
            return true;
        };
        ENSURE(!llvm::verifyModule(*module, &llvm::errs()) || !print_newline());
    }

    bool dump_llvm = dump_llvm_opt.present_or_true();
    if (dump_llvm) {
        module->print(llvm::errs(), nullptr);
    }

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();
    if (run) {
        auto *function = module->getFunction("main");
        ENSURE(function != nullptr);
        llvm::EngineBuilder engine_builder(std::move(module));
        engine_builder.setEngineKind(llvm::EngineKind::Either);
        std::unique_ptr<llvm::ExecutionEngine> engine(engine_builder.create());
        return engine->runFunctionAsMain(function, {"hello"}, nullptr);
    }
    llvm::TargetOptions options;
    const auto *target = &*llvm::TargetRegistry::targets().begin();
    auto *machine = target->createTargetMachine(llvm::sys::getDefaultTargetTriple(), llvm::sys::getHostCPUName(), "",
                                                options, llvm::Reloc::DynamicNoPIC);
    std::error_code ec;
    llvm::raw_fd_ostream output("out.o", ec, llvm::sys::fs::OF_None);
    llvm::legacy::PassManager pm;
    machine->addPassesToEmitFile(pm, output, nullptr, llvm::CodeGenFileType::CGFT_ObjectFile);
    pm.run(*module);
    output.flush();
}
