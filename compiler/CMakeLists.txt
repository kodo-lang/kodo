add_executable(compiler
    ast/Dumper.cc
    ast/Nodes.cc
    ir/BasicBlock.cc
    ir/Constants.cc
    ir/Dumper.cc
    ir/Function.cc
    ir/Instruction.cc
    ir/Instructions.cc
    ir/Program.cc
    ir/Value.cc
    pass/PassManager.cc
    support/ArgsParser.cc
    support/Assert.cc
    CharStream.cc
    IrGen.cc
    Lexer.cc
    LLVMGen.cc
    main.cc
    Parser.cc
    Token.cc
    Type.cc
    TypeChecker.cc
    VarChecker.cc)
target_compile_definitions(compiler PRIVATE ${LLVM_DEFINITIONS})
target_compile_features(compiler PRIVATE cxx_std_20)
target_include_directories(compiler PRIVATE . ${LLVM_INCLUDE_DIRS})
target_link_libraries(compiler PRIVATE fmt::fmt LLVM)
