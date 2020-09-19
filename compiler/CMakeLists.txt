add_executable(compiler
    Analyser.cc
    Ast.cc
    CharStream.cc
    CodeGen.cc
    Lexer.cc
    main.cc
    Parser.cc
    Token.cc
    Type.cc)
target_compile_definitions(compiler PRIVATE ${LLVM_DEFINITIONS})
target_compile_features(compiler PRIVATE cxx_std_20)
target_include_directories(compiler PRIVATE . ${LLVM_INCLUDE_DIRS})
target_link_libraries(compiler PRIVATE fmt::fmt LLVM)
