# Nicholas Mosier 2017
# CS433 Programming Assignment 4: Semantic Analysis

## Files

Your directory should contain the following files:

1. `CMakeLists.txt`: CMake file for building the semantic analyzer. You should
   not need to modify this file.
1. `semant-main.cc`: Executable for your semantic analyzer. You should not need
   to modify this file.
1. `mysemant`: A shell-script to connect your semantic analyzer to the
   reference lexer and parser to facilitate testing.
1. `good.cl`, `bad.cl`: Files to test a few features of the semantic analyzer.
   You should add tests to ensure that good.cl exercises as many legal semantic
   combinations as possible and that bad.cl exercises as many different
   semantics errors as you can incorporate into one file. If you can't
   integrate all of your tests into these files, or want to break your test
   suite into multiple files, you can also use the integration test
   infrastructure in the `test/integration` directory, or add more files to
   your project in this assignment directory.
1. `README.md`: This file. You will need to edit the README file with your
   write-up.

Unlike past assignments, you will also need to modify files in the `src`
directory. Specifically:

1. `src/include/ast.h`: Header for AST nodes
1. `src/include/semant.h`: Header for classes and functions used in semantic
   analysis
1. `src/semant.cc`: Implementation files for classes and functions used in
   semantic analysis

You may find the support classes in `src/include/ast_consumer.h` helpful, but
you are not obligated to use that code.

## Instructions

To build the semantic analyzer
```
make semant
```
and test it in isolation:
```
./mysemant good.cl
```

You can compare the output of your parser to that of the reference
implementation
```
./mysemant -S ../bin/semant good.cl
```
or use your lexer as part of the entire Cool compiler.
```
../bin/mycoolc -S ./semant good.cl
```

To enable optional debugging add `-s` after a `--`, e.g.
```
./mysemant -- -s good.cl
```

Note that this flag just sets the global variable `cool::gSemantDebug` to be true.
You will need to add your own debugging messages (guarded by this flag), e.g.:

```
if (cool::gSemantDebug) {
    std::cerr << "Something unexpected happened in my program" << std::endl;
}
```

Instructions for turning in the assignment will be posted on the course site.
Make sure to complete the writeup below.

## Write-up

My semantic analyzer consists of 3 major components: the class inheritance graph (InheritanceGraph), the semantic environment (SemantEnv), and typechecking routines for abstract syntax tree nodes. The general order of execution of the semantic analyzer is the following: the inheritance graph is constructed and checks for inconsistencies or errors in inheritance are performed. Next, the semantic environment is set up, at which point the scoped method tables are constructed and method-declaration related type errors are reported. Control is then passed to the typechecking methods, implemented in the AST nodes, via the bridge function, TypeCheckClass(), of SemantEnv, which recursively initiates typechecking of each class by traversing the inheritance graph from the root node (Object).

The first step of the program, creating and verifying the inheritance graph, occurs in three substages: first, all class nodes of the AST, in addition to the built-in classes such as Object, are added to the graph as isolated inheritance graph nodes (the routine checks for class redefiniton at this stage). The inheritance graph itself is based on the KlassTable provided in ast_consumer.h. Each inheritance graph node is of (aptly named) type InheritanceGraphNode, which is an extension of the InheritanceNode provided in ast_consumer.h, and corresponds to a single class in the AST. Once all nodes have been added to the graph, the next stage is to connect the isolated nodes together according to inheritance, which is performed in the ConnectNodes() method. At this step, inheritance from undefined or uninheritable classes checked for. The third stage checks for cycles within the inheritance graph, implemented in the CheckAcyclic() function. This catches errors of self-inheritance, inheritance from an ill-defined class, and inheritance cycles. All classes with any of these type errors are marked as invalid, which is information that is used later during typechecking.
    I decided to continue semantic analysis even if errors are encountered in the inheritance graph. Although this introduced more room for errors later on, the semantic environment and AST typechecking take this into consideration and perform the necessary checks on type identifiers as necessary.
    Also provided by the inheritance graph are methods for checking the relationship between types; specifically, it implements the InheritsFrom() and LeastUpperBound() methods (note: the semantic environment builds off of these).
    
The next step of semantic analysis is creating the sematic environment, encapsulated in the SemanticEnv object. The first substage is creating the scoped method tables for all (valid) classes in the inheritance graph. The approach I implemented was recursive, starting at the root node (Object). At the recursive call on each inheritance node, the inherited method table is copied, a new scope is entered, and all the current class' methods are added. The method is then called on children of the current node in the inheritance graph recursively. All scoped method tables are collected into a ScopedMethodTables object, which serves at the 'M' environment. At this stage, method redefinition within the same scope, mismatches in overloaded method signatures, illegal formals, and the existence of a main method are checked for. Following this step, each class is prepared to be typechecked (recursively, starting at the root node). This involves adding all attributes to the 'O' environment (as well as SELF_TYPE/self) and then typechecking method bodies, effected by the TypeCheck(O, M, C) method of each Expression node in the AST.
    
A typechecking method that accepts the inheritance graph, the semantic environment, and the current classs is implemented for each expression node in the AST. Each method recursively calls the typechecking methods of any children expressions and sets its own type (type_) according to the type rules in the Cool manual. For some types of expressions, special typechecking is necessary, for example in case branches, where newly introduced identifiers cannot be bound to SELF_TYPE. To verify type relationships (i.e. the less-than-or-equal-to and least-upper-bound functions), the typechecking methods use the methods provided by the semantic environment, which are designed to handle No_type as well as SELF_TYPE.
    
As for error recovery, I implemented a combination of the simple, panic-mode equivalent of upon an error setting the expression's type to Object and setting the type to No_type, which effectively silences type errors. I think this approach ultimately achieved a greater balance of informativeness of errors than going with exclusively one solution or the other would have.
    
To test my semantic analyzer, I added to the good.cl and bad.cl source files in addition to running it on all example source files in the "examples" directory. Because my semantic analyzer does not abort when it encounters an error in the inheritance graph, I could fit almost all the errors into the single bad.cl test file. The two errors that are mutually exclusive are "missing main method" and "no class Main" errors; I specifically verified that the semantic analyzer detected each error correctly. In the good.cl source file, I recreated a handful of specific situations that I would not expect to occur in any of the files in the examples directory, such as introducing a series of identical identifiers of different types in a let expression and the mutual reference of attributes in their respective initializers (e.g. attr1 : Int <- attr2 \n attr2 : Int <- attr1) (which doesn't seem to be verboten in the Cool manual and is accepted by the reference semantic analyzer). Other than such cases, for verifying that my semantic analyzer works correctly on valid Cool programs, I relied mostly on the (certaintly/hopefully exhaustive) trove of valid code in the "examples" directory.
