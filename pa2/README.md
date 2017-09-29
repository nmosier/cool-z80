# CS433 Programming Assignment 2: Lexing

## Files

Your directory should contain the following files:

1. `CMakeLists.txt`: CMake file for building the lexer. You should not need to
   modify this file.
1. `lexer-main.cc`: Executable for your lexer. You should not need to modify this file.
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

I grouped the translation rules in the cool.flex file by what types of tokens they correspond to; for example, all the keywords and operators are grouped together. In some cases, the precedence of some rules over another required a specific ordering of the translation rules, e.g. the rule to consume the remainer of a comment must come after the rule that handles a newline within a comment.
    In order to make handling multi-line comments and strings more tractible, I defined names for regular expressions for each. Specifically, I defined the "nested_safe" regular expression, which accepts any string that a '(', '*', or ')' can be appended to without inadvertently forming a tailing '*)' or '(*' sequence. This allows the comment-closing sequences '(*' and '*)' to behave consistenly and to only correspond to true comment openings/closures when used in the regular expressions for multi-line comments in the translation rules section of cool.flex. Additionally, I declared an integer variable to keep track of the nest level of multi-line comments.
    I also declared start conditions for string literals and multi-line comments, since the languages they accept differ greatly from the rest of the Cool languge.
    Lexing the string literal token required the most implementation-heavy action, since it had to properly replace different types of escape sequences and enforce a maximum length.
    
    I created five test files in addition to the 'test.cl' file provided with the assignment skeleton. They are the following:
    - test_int.cl: this tests the lexer's ability to lex integer constants per the Cool specification. It includes cases for testing the integer's max value, 2147483647, and one past its max value, 2147483648. It also tests the lexer's handling of integers with leading zeroes and integers within comments.
    - test_str.cl: this tests the lexer's ability to lex string constants. It includes variations of string literals 1024 characters in length, some of which include escape sequences that, when not replaced with the corresponding single ASCII character, would push the length over 1024 charcters. It also includes strings that are 1025 characters long, strings within comments, and both escaped and non-escaped newline characters within strings. There are also test cases for the null character and both generic as well as special escape sequences.
    - test_comments.cl: this tests the lexer's ability to recognize and skip over comments. It tests both single-line as well as multi-line comments and creates situations that will generate all possible comment-related errors. Also, it includes a painful-to-look-at agglomeration of '*', '(', and ')' characters in order to test whether it is truly robust.
    - test_keywords.cl: this tests the lexer's ability to lex keywords in the Cool language as well as its operators and boolean literals. It includes three sets of all keywords, each in a different case (uppercase, lowercase, capitalized). It also tests all capitalization rules for boolean literals (they cannot be capitalized). The test file includes all single-character as well as double-character operators. The lexers behavior around disallowed characters is also tested. Finally, potentially ambiguous combinations of operators (e.g. "<=>") are included to verify the model lexer and my implementation behave identically.
    - test.cl: I did not modify this test file, but still used it to test my lexer.
    - test_mega.cl: this is a concatenation of all Cool source files in the "examples" directory. The sheer size of source material confirms that my lexer and the lexer provided tokenize Cool programs identically in practice.
    To test my lexer, I put all the test files in a folder called "tests" within the "pa2" directory and used a short bash for-loop to tokenize the each Cool test file with my lexer and the provided lexer, pipe the tokenized output to separate files, and "diff" the results. Doing this does not cause "diff" to print anything, confirming my implementation of the lexer is adequate (for the tests provided).
