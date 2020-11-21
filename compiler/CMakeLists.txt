add_executable(compiler
    analyses/ControlFlowAnalysis.cc
    analyses/ReachingDefAnalysis.cc
    ast/Dumper.cc
    ast/Nodes.cc
    ir/BasicBlock.cc
    ir/Constants.cc
    ir/Dumper.cc
    ir/Function.cc
    ir/Instruction.cc
    ir/Instructions.cc
    ir/Program.cc
    ir/TypeCache.cc
    ir/Types.cc
    ir/Value.cc
    pass/PassManager.cc
    support/ArgsParser.cc
    support/Assert.cc
    support/Error.cc
    CharStream.cc
    Compiler.cc
    IrGen.cc
    Lexer.cc
    LLVMGen.cc
    main.cc
    Parser.cc
    StackPromoter.cc
    Token.cc
    TypeChecker.cc
    VarChecker.cc)
configure_file(Config.hh.in Config.hh)
target_compile_definitions(compiler PRIVATE ${LLVM_DEFINITIONS})
target_compile_features(compiler PRIVATE cxx_std_20)
target_include_directories(compiler PRIVATE
    .
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LLVM_INCLUDE_DIRS})
target_link_libraries(compiler PRIVATE fmt::fmt LLVM)
