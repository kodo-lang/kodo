add_executable(kodoc
    analyses/ControlFlowAnalysis.cc
    analyses/ReachingDefAnalysis.cc
    ir/BasicBlock.cc
    ir/Constants.cc
    ir/Dumper.cc
    ir/Function.cc
    ir/GlobalVariable.cc
    ir/Instruction.cc
    ir/Instructions.cc
    ir/Prototype.cc
    ir/TypeCache.cc
    ir/Types.cc
    ir/Value.cc
    pass/PassManager.cc
    support/ArgsParser.cc
    support/Assert.cc
    support/Error.cc
    CharStream.cc
    Compiler.cc
    ConcreteImplementer.cc
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
target_compile_definitions(kodoc PRIVATE ${LLVM_DEFINITIONS})
target_compile_features(kodoc PRIVATE cxx_std_20)
target_include_directories(kodoc PRIVATE
    .
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LLVM_INCLUDE_DIRS})
target_link_libraries(kodoc PRIVATE fmt::fmt LLVM)
install(TARGETS kodoc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
