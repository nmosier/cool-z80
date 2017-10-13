# CS433 Programming Assignment 3: Parsing

## Files

Your directory should contain the following files:

1. `CMakeLists.txt`: CMake file for building the parser. You should not need to
   modify this file.
1. `parser-main.cc`: Executable for your parse. You should not need to modify
   this file.
1. `myparser`: A shell-script to connect your parser to the reference lexer to
   facilitate testing.
1. `cool.y`: A skeleton file for the Bison specification. It already contains
   productions for the program and the classes. Use them as an example to write
   the remaining productions.
1. `good.cl`, `bad.cl` test a few features of the grammar. You should add tests
   to ensure that good.cl exercises every legal construction of the grammar and
   that bad.cl exercises as many different parsing errors as you can squeeze
   into one file. If you can't integrate all of your tests into these files, or
   want to break your test suite into multiple files, you can also use the
   integration test infrastructure in the `test/integration` directory.                                                        	
1. `README.md`: This file. You will need to edit the README file with your
   write-up.                                                          	

## Instructions

To build the parser
```
make parser
```
and test it in isolation:
```
./myparser good.cl
```

You can compare the output of your parser to that of the reference implementation
```
./myparser -P ../bin/parser good.cl
```
or use your lexer as part of the entire Cool compiler.
```
../bin/mycoolc -P ./parser test.cl
```

To enable the detailed Bison debugging information add `-p` after a `--`, e.g.
```
./myparser -- -p good.cl
```

Instructions for turning in the assignment will be posted on the course site. Make sure to complete the writeup below.


## Write-up

Nicholas Mosier
10.13.2017

    -Design decisions
    In general, I implemented the grammar rules in the same order in which they appear in the Cool manual. Within the rule that defines the 'expression' non-terminal, I wrote the productions in reverse order, which allowed for testbale partial functionality before the entire 'expressuin' rule was complete. Some non-terminals (such as 'class', 'feature', etc) required the introduction of additional supporting rules, mainly for recognizing lists of non-terminals (e.g. formals) in the grammar. When designing the productions for such non-terminals, I used the same tail-recursive form as used in the rule for 'class_list'.
    One important design decision was my implementation of the 'let' statement. The 'Let::Create' function allows for the declaration of only one new object identifier; however, the Cool manual specifies that the 'let' statement can have one or more declarations. In order to create the effect of multiple variable declarations in a single let statement, I wrote a grammar rule that sequentially chains multiple 'let' statements together, one per declaration. Since each 'let' statement creates its own local scope, these chained 'let' statements form a stack of scopes in which any preceding identifier is visible to any subsequently declared identifier. This behavior is exactly what the Cool manual specifies, and this approach will simplify the implementation of the semantic analyzer for 'let' statements.
    Resolving the ambiguity introduced by the 'let' statement required another important design decision. As suggested, I used bison's support for declaring precedence for non-operator terminals. Since right-associativity corresponds to a preference for shifting and the precedence of a rule is set by last token, I declared the terminal 'IN' to have lowest precedence and to be right-associative. To test my logic, I also tried declaring 'IN' with highest precedence; as expected, during parsing the 'let' statement extended minimally opposed to maximally, as defined in the Cool manual.
    
    -Parsing errors
    As specified in the assignment description, the parser needs to recover from a syntactically incorrect class, feature, and expression inside a block. In order to implement panic-mode error recovery, each case needs a syncronizing token, which happens to be a ';' for all three. Furthermore, since classes, features, and expressions in a block each occur in respective lists, the approach for catching and recovering from errors is essentially the same: each corresponding list non-terminal can derive an error followed by ';', or itself followed by an error and ';'. This suffices to catch and recover from all errors in class, feature, and expression lists.
    The parser is also required to recover from an error in a 'let' binding, which called for a different approach, as 'let' statements with multiple bindings are implemented differently than the lists seen above. When an error is encountered in a 'let' binding list, if it is the last binding, then the node of the body expression should simply be assigned to the current AST node. If it followed by more bindings, then it the following 'let' statement node should be assigned to the current AST node. In these two cases, 'IN' and ',' function as the syncronizing tokens.
    
    -Sufficiency of tests
    The tests in "good.cl" are sufficient because it includes more than one class, contains multiple attributes and methods, and an example of every production in the Cool syntax specification. For example, it contains methods with 0, 1, and multiple formals; similarly, it contains 'let' statements of one and of multiple bindings. My parser parses 'good.cl' without generating any errors or differences with the refernece parser output.
    The tests in "bad.cl" test error recovery from when errors occur within classes, features, block expressions, and 'let' statement bindings. Additionally, it includes a syntactically incorrect usage of all (would-be) productions in the Cool syntax specification, e.g. operator is used incorrectly at least once. My parser catches all the errors in 'bad.cl' and subsequently recovers where it should, producing no differences in the error output against the reference parser's.
    I also checked my parser's results against the reference parser's results for all the example Cool source files, and the outputs were identical.
