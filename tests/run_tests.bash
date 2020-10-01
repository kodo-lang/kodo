#!/bin/bash

COMPILER=$1
run_test() {
    printf "Running test '%s' " $1
    OUTPUT=$($COMPILER --silent $(dirname $0)/$1)
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

run_test "implicit_extension.lang" 10 ""
run_test "libc_hi.lang" 0 "Hi"
run_test "malloc.lang" 0 "A"
run_test "unknown_symbols.lang" 1 "error: no symbol named 'bar' in current context on line 2
error: no function named 'test' in current context on line 2
 note: Aborting due to previous errors"
