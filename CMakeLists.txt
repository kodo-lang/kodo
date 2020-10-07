cmake_minimum_required(VERSION 3.16)
project(lang)

find_package(fmt REQUIRED)
find_package(LLVM REQUIRED CONFIG)
add_subdirectory(compiler)
add_subdirectory(tests)
