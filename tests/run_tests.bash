#!/bin/bash

COMPILER=$1
run_test() {
    printf "Running test '%s' " $1
    OUTPUT=$($COMPILER --silent $(dirname $0)/$1)
    if [ $2 -ne $? ] || [ "$3" != "$OUTPUT" ]
    then
        printf "\u001b[31mFAILED\u001b[0m\n"
        OUTPUT=${OUTPUT::-5}
        while IFS= read -r line; do
            printf "  %s\n" "$line"
        done <<< "$OUTPUT"
        printf "\u001b[0m"
    else
        printf "\u001b[32mOK\u001b[0m\n"
    fi
}

run_test "implicit_extension.lang" 10 ""
run_test "libc_hi.lang" 0 "Hi"
run_test "malloc.lang" 0 "A"
