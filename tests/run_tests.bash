#!/bin/bash

COMPILER=$1
run_test() {
    printf "Running test '%s' " $1
    OUTPUT=$($COMPILER run $(dirname $0)/$1)
    RET=$?
    OUTPUT_STRIPPED=$(echo "$OUTPUT" | sed 's/\x1b\[[0-9;]*m//g')
    if [ $2 -ne $RET ] || [ "$3" != "$OUTPUT_STRIPPED" ]
    then
        printf "\u001b[31mFAILED\u001b[0m\n"
        printf "%s" "$(echo "$OUTPUT" | sed 's/^/  /g')"
        printf "\u001b[0m"
    else
        printf "\u001b[32mOK\u001b[0m\n"
    fi
}

# Expecting compile error.
run_test "compile-error/bad_if.kd" 1 "error: cannot implicitly cast from 'i32' to 'bool' on line 2
 note: Aborting due to previous errors"
run_test "compile-error/bad_mutability.kd" 1 "error: attempted assignment of immutable variable 'bar' on line 2
error: attempted assignment of immutable variable 'foo' on line 7
 note: Aborting due to previous errors"
run_test "compile-error/bad_pointer_mutability.kd" 1 "error: cannot implicitly cast from '*i32' to '*mut i32' on line 10
error: attempted assignment of 'i32' value pointed to by an immutable pointer on line 2
 note: Aborting due to previous errors"
run_test "compile-error/type_errors.kd" 1 "error: 'test' requires 2 arguments, but 0 were passed on line 7
error: cannot implicitly cast from 'i32' to '*mut i32' on line 8
error: cannot implicitly cast from '*i32' to 'i32' on line 8
 note: Aborting due to previous errors"
run_test "compile-error/unknown_symbols.kd" 1 "error: no symbol named 'bar' in current context on line 2
error: no function named 'test' in current context on line 2
 note: Aborting due to previous errors"
run_test "compile-error/use_before_init.kd" 1 "error: use of possibly uninitialised variable 'a' on line 3
error: use of possibly uninitialised variable 'c' on line 8
 note: Aborting due to previous errors"

# Expecting success.
run_test "success/basic_struct.kd" 3 ""
run_test "success/comparison.kd" 1 ""
run_test "success/complex_expression.kd" 55 ""
run_test "success/complex_struct.kd" 24 ""
run_test "success/const_decl.kd" 20 ""
run_test "success/hello_world.kd" 0 "Hello, world!"
run_test "success/implicit_extension.kd" 10 ""
run_test "success/inline_asm.kd" 0 "Hello, world!"
run_test "success/instance_member_function.kd" 20 ""
run_test "success/libc_hi.kd" 0 "Hi"
run_test "success/malloc.kd" 0 "A"
run_test "success/mutability.kd" 20 ""
run_test "success/pointer_mutability.kd" 70 ""
run_test "success/simple_if.kd" 0 "AAA"
run_test "success/static_member_function.kd" 5 ""
run_test "success/type_alias.kd" 5 ""
