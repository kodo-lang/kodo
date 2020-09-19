cmake_minimum_required(VERSION 3.17)
project(lang)

find_package(fmt REQUIRED)
find_package(LLVM REQUIRED CONFIG)
add_subdirectory(compiler)
