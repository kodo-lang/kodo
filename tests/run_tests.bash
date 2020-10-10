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

# Expecting success.
run_test "comparison.lang" 1 ""
run_test "complex_expression.lang" 55 ""
run_test "hello_world.lang" 0 "Hello, world!"
run_test "implicit_extension.lang" 10 ""
run_test "libc_hi.lang" 0 "Hi"
run_test "malloc.lang" 0 "A"
run_test "mutability.lang" 20 ""
run_test "simple_if.lang" 0 "AAA"

# Expecting compile error.
run_test "bad_if.lang" 1 "error: cannot implicitly cast from 'i32' to 'bool' on line 2
 note: Aborting due to previous errors"
run_test "bad_mutability.lang" 1 "error: attempted assignment of immutable variable 'foo' on line 3
 note: Aborting due to previous errors"
run_test "type_errors.lang" 1 "error: 'test' requires 2 arguments, but 0 were passed on line 9
error: cannot implicitly cast from 'i32' to 'i32*' on line 10
error: cannot implicitly cast from 'i32*' to 'i32' on line 10
 note: Aborting due to previous errors"
run_test "unknown_symbols.lang" 1 "error: no symbol named 'bar' in current context on line 2
error: no function named 'test' in current context on line 2
 note: Aborting due to previous errors"
run_test "use_before_init.lang" 1 "error: use of variable 'a' before initialization on line 6
 note: Aborting due to previous errors"
