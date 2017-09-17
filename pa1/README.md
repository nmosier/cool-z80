# CS433 Programming Assignment 1: Introduction to Cool
# Nicholas Mosier
# 09.17.2017

## Write-up

1. Describe your implementation of the stack machine in a single short paragraph.
    My object-oriented implementation of the stack machine is driven by the interaction between two classes: the Stack and the StackCommand. The Stack class is implemented as a linked list of instances of the Stack_Node class, which stores an instance of StackCommand and a reference to the next Stack_Node on the stack. The interface of Stack is only two functions, push(cmd : StackCommand_Pushable) and pop(). The StackCommand object is a base class for all commands that declares necessary methods for operating on a Stack object and printing the command itself to standard output. A second class, StackCommand_Pushable, inherits from StackCommand and forms the base class for all stack commands that can be pushed onto the stack (i.e. any integer, "+", and "s"). All commands inherit from either the StackCommand or StackCommand_Pushable class and redefine the evaluation and output functions. The eval(s : Stack) : OptionalInt method of any subclass performs its apprpriate operation on the Stack object and returns an Int (accessible through the OptionalInt's get_value() method) if it results in a value or Void otherwise. When operating on a stack, StackCommands never push themselves or new values onto the stack; this is handled in the main input loop. The main input loop drives the program by repeatedly (until "x" is encountered) prompting for input, creating a StackCommand from the input, either pushing it to the stack or evaluating it (and possibly pushing back a non-Void result) after checking its subtype.
    

2. List 3 things that you like about the Cool programming language.
    - I like the fact that every statement (e.g. the while loop) is an expression. It seems to make the language more simiplified and unified.
    - Cool makes inheritance easier to work with and understand, at least in my experience.
    - Arguments a dispatch expression and initializations of a "let" expression are evaluated in left-to-right order. In C, the evaluation order is undefined (I think). Although this doesn't matter most of the time, it could be useful some situations.

3. List 3 things you *don't* like about Cool.
    - There is no way to define a class initializer that is automatically called when a new object is instantiated.
    - The built-in String class offers no member method for checking equality. Instead, the '=' operator must be used. When comparing user-defined classes, though, the '=' operator checks for object identity, not the equality of its contents, which represents an inconsistency in usage.
    - Because expressions in Cool have a greater tendency to be nested than in other languages, like C, indentations can get out of hand and often don't serve the purpose of making code easier to follow.
