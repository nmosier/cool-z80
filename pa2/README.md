# CS433 Programming Assignment 2: Lexing

## Files

Your directory should contain the following files:

1. `CMakeLists.txt`: CMake file for building the lexer. You should not need to
   modify this file.
1. `cool.flex`: A skeleton file for the Flex specification. You should complete
   this file with your regular expressions, patterns and actions (and any
   relevant support code you need).
1. `test.cl`: A COOL program that you can use to test the lexical analyzer. It
   contains some errors, so it won't compile with `coolc`. However, `test.cl`
   does not exercise all lexical constructs of COOL and part of your assignment
   is to rewrite `test.cl` with a more complete set of tests for your lexical
   analyzer. If you can't integrate all of your tests into a single file, or
   want to break your test suite into multiple files, you can also use the
   integration test infrastructure in the `test/integration` directory.
1. `README.md`: This file. You will need to edit the README file with your
   write-up. 

As part of the assignment you likely need to consult the files in the `src`
directory, but should not need to modify them (for this assignment).

## Instructions

To build the lexer
```
make lexer
```
and test it in isolation:
```
./lexer test.cl > test.tokens
```

You can compare the output of your lexer to that of the reference lexer
```
../bin/lexer test.cl
```
or use your lexer as part of the entire Cool compiler.
```
../bin/mycoolc -L ./lexer test.cl
```

Instructions for turning in the assignment will be posted on the course site. Make sure to complete the writeup below.

## Write-up

Replace with a brief write-up of your implementation as described in the assignment PDF.
