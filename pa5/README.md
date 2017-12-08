# CS433 Programming Assignment 5: Code Generation

## Files

Your directory should contain the following files:

1. `CMakeLists.txt`: CMake file for building the code generator. You should not
   need to modify this file.
1. `cgen-main.cc`: Executable for your code generator. You should not need to
   modify this file.
1. `mycoolc`: A bash-script to connect your code generator to the reference
   components to facilitate testing.
1. `example.cl`: This file should contain a test program of your own design.
   Test as many features of the code generator as you can. You can also use the
   integration test infrastructure in `test/integration`, or add more files to
   your project in this assignment directory. 
1. `README.md`: This file. You will need to edit the README file with your
   write-up.

Like PA4 you will also need to modify files in the `src` directory.
Specifically:

1. `src/include/ast.h`: Header for AST nodes
1. `src/include/cgen.h`: Header for classes and functions used in code generation
1. `src/cgen.cc`: Implementation files for classes and functions used in code generation

You may find the support code in `src/emit.h` and `src/include/ast_consumer.h`
helpful, but you are not obligated to use that code.

## Instructions

To build the code generator
```
make cgen
```
and test it in isolation:
```
./mycoolc example.cl
```

The above will generation `example.s`. To run the corresponding assembly file with SPIM:

```
../bin/spim -- -file example.s
```

*Note* the double dashes: these are required and separate the arguments to the
script that runs `cool-spim` (the actual simulator executable) and the
arguments to `cool-spim` itself. 

You can compare the output of your code generator to that of the reference
implementation:

```
./mycoolc -C ../bin/cgen example.cl
../bin/spim -- -file example.s
```

Your assembly is not expected to match that produced by the reference
implementation, but the generated code should produce the same results. Thus to
test your code generator you will need to write Cool programs that generate
some kind of output during execution (e.g. print the result of a computation).

To enable optional debugging add `-c` after a `--`, e.g.
```
./mycoolc -- -c example.cl
```

Note that this flag just sets the global variable `cool::gCgenDebug` to be true.
You will need to add your own debugging messages (guarded by this flag), e.g.:

```
if (cool::gCgenDebug) {
    std::cerr << "Something unexpected happened in my program" << std::endl;
}
```

Instructions for turning in the assignment will be posted on the course site.
Make sure to complete the writeup below.

## Write-up

- memory locations
- activation record
- determining # of temporaries
- case statement
- initialization

Overall, my code generator proceeds as follows: it constructs the inheritance graph, dispatch tables, and intial variable environments; it emits prototype objects and all tables; it emits initializers; finally, it emits the methods. Most of these steps are performed recursively.

To encapsulate the variable environment, I defined a new class (VariableEnvironment) that maps identifiers (symbols) at different scopes to memory locations. A variable environment for each node on the inheritance graph is constructed, containing first only attributes. This environment is then passed to the initializer and method code generation phases for the current class.

To encapsulate a generic memory location, I defined the MemoryLocation base class, which I use to represent the addresses of variables as well as the addresses of method entry points. During code generation and execution, memory locations are addressed in two ways: directly or indirectly. For each possibility, I created a specialized class (AbsoluteLocation and IndirectLocation) that inherits from MemoryLocation and provides overridden methods for emitting code that reads to/from (and obtains the addresses of) the locations. This greatly simplified writing the code generation code — it abstracted away the need to check whether to access an identifier directly (though I don't think this would ever be the case), or if indirectly, which base register to use (SELF, FP, etc).

My design of the activation record changed many more times than it should as I implemented the code generator. Before writing code, I planned out a standard AR setup containing the caller's return address, self, and frame pointer. While debugging at some point along the way while writing the code, though, I ended up for some reason or another removing the return address from the AR and simply preserving it throughout a method. Although this has the same effect as storing it in the AR, it is far less efficient to preserve the $ra register than to simply restore from the AR at the end of a method. If I were to work on this code generator further, this would be one of the first aspects I would change.

My approach to reserving space for temporaries (e.g. those introduced by let and case statements) consists of two components: the first calculates the number of temporaries an expression requires (without generating any code); the second assigns memory locations (on the stack) to temporaries during the recursive code generation phase for expressions. The temporaries' identifiers are mapped to their memory locations in the recursively passed variable environment class and removed once the introducing expression's code has been generated. To calculate the number of required temporaries, I added a new overridden method to the ASTNode class that performs the calculation for each type of expression (e.g. for "let", the max no. of temporaries is the max over the no. for the initializer and for the no. for the body plus 1).

In implementing code generation for the case statement, instead of somehow storing the inheritance graph as a global tree in the data segment of the code, I opted for a simpler approach of outputting lists of qualifying tag ID's for each case branch in each case statement. For each case statement, my code generator outputs a series of such lists, one per case branch. Each list consists of tag of the case branch's declared type along with the tags of all types that inherit from that type. The code for the case statement scans through these segments linearly but is required to find the "least" (i.e. furthest down the inheritance graph)  type among the case branches that the case expression inherits from. To ensure correct behavior, it suffices order the tag sets by increasing size, since (i) if one branch type inherits from another branch type, its set size will always be smaller, and (ii) if neither branch type inherits from the other, then the case expression's type can only match at most one of the branches.
    There is defintely room for optimization in my approach to case statements — for example, my code generator naïvely outputs redundant tags that had already been checked against for previous branches.

In writing the code generation methods for the AST nodes, I paid attention to the order of evaluation specified in the Cool Manual. For example, a dispatch expressions' actuals are evaluated first (from left to right) before the receiver object is evaluated. Similarly, the identifier of a let statement is neither introduced into the scope nor mapped to a temporary before the code for its initializer is generated.

To test my code generator, I compiled each program in the examples directory using the reference code generator as well as my own, and then compared program behaviors. In "pa5/example.cl" I added additional tests for corner cases not covered by the programs in the examples directory (such as dispatches where order of execution matters); in "pa5/errors.cl" I added methods to test each of the three runtime errors the code generator is responsible for catching.
