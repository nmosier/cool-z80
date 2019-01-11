(*  Nicholas Mosier 2017
 *  CS433
 *
 *  Implementation of a simple stack machine.
 *)
 
class Main inherits IO {
    converter : A2I <- new A2I;
     
    main() : Object {
        let input : String, stack : Stack <- new Stack in
            while not (input = "x")
            loop {
                out_string(">");
                input <- in_string();
                let cmd : StackCommand <- make_cmd(input) in
                    case cmd of
                        pushable : StackCommand_Pushable => stack.push(pushable);
                        aux : StackCommand => let val : OptionalInt <- aux.eval(stack) in
                            if isvoid val then
                                val
                            else
                                stack.push((new StackCommand_Int).init(val.get_value()))
                            fi;
                    esac;
            } pool         
     };
     
     make_cmd(str : String) : StackCommand {
         if str = "+" then new StackCommand_Add else
         if str = "s" then new StackCommand_Swap else
         if str = "e" then new StackCommand_Eval else
         if str = "d" then new StackCommand_Disp else
         if str = "x" then new StackCommand_Exit else
             (new StackCommand_Int).init(converter.a2i(str))
         fi fi fi fi fi
     };
     
 };
 
 class OptionalInt {
     value : Int;
     init(val : Int) : OptionalInt {{ value <- val; self; }};
     get_value() : Int { value };
 };
 
 
 class Stack_Node {
     head : StackCommand_Pushable;
     tail : Stack_Node;
     
     get_head() : StackCommand_Pushable { head };
     get_tail() : Stack_Node { tail };
     
     set_head(cmd : StackCommand_Pushable) : Stack_Node {{ head <- cmd; self; }};
     set_tail(node : Stack_Node) : Stack_Node {{ tail <- node; self; }};
     
 };
 
 class Stack {
     top : Stack_Node;
     
     push(cmd : StackCommand_Pushable) : Stack {{
         top <- (new Stack_Node).set_tail(top).set_head(cmd);
         self;
     }};

     pop() : StackCommand_Pushable {
         if isvoid top then
             let ret_val : StackCommand_Pushable in ret_val
         else
             let cmd : StackCommand_Pushable <- top.get_head() in {
                 top <- top.get_tail();
                 cmd;
             } fi
     };
     
 };

class StackCommand {
     eval(stack : Stack) : OptionalInt {
         let ret_val : OptionalInt in ret_val
     };

     out_cmd(os : IO) : IO {
         os.out_string("\n")
     };     
};
 
class StackCommand_Pushable inherits StackCommand {};

class StackCommand_Int inherits StackCommand_Pushable {
    value : Int;
    
    init(val : Int) : StackCommand_Int {{ value <- val; self; }};
    get_value() : Int { value };
    
    eval(stack: Stack) : OptionalInt { (new OptionalInt).init(value) };
    out_cmd(os : IO) : IO {{ os.out_int(value); os.out_string("\n"); }};
    
};

class StackCommand_Add inherits StackCommand_Pushable {
    eval(stack: Stack) : OptionalInt {
        let op1 : OptionalInt, op2 : OptionalInt in {
            while isvoid op1
            loop
                op1 <- stack.pop().eval(stack)
            pool;
         
            while isvoid op2
            loop
                op2 <- stack.pop().eval(stack)
            pool;

            let sum : Int <- op1.get_value() + op2.get_value() in
                (new OptionalInt).init(sum);
         }
     };
     
     out_cmd(os : IO) : IO { os.out_string("+\n") };
     
 };

 class StackCommand_Swap inherits StackCommand_Pushable {
     eval(stack : Stack) : OptionalInt {
         let cmd1 : StackCommand_Pushable <- stack.pop(), cmd2 : StackCommand_Pushable <- stack.pop(), ret_val : OptionalInt in {
             stack.push(cmd1);
             stack.push(cmd2);
             ret_val;
         }
     };
     out_cmd(os : IO) : IO { os.out_string("s\n") };
     
 };
 
 class StackCommand_Eval inherits StackCommand {
     eval(stack : Stack) : OptionalInt {
         let cmd : StackCommand_Pushable <- stack.pop(), ret_val : OptionalInt in
             if not isvoid cmd then
                 cmd.eval(stack)
             else
                 ret_val
             fi
     };
     out_cmd(os : IO) : IO { os.out_string("e\n") };
 };

class StackCommand_Disp inherits StackCommand {
    eval(stack : Stack) : OptionalInt {
        let os : IO <- (new IO), ret_val : OptionalInt in {
            out_stack(os, stack);
            ret_val;
        }
    };
    
    out_cmd(os : IO) : IO { os.out_string("d\n") };

    out_stack(os : IO, stack : Stack) : Stack {
        let cmd : StackCommand_Pushable <- stack.pop() in
            if isvoid cmd then stack
            else {
                cmd.out_cmd(os);
                out_stack(os, stack);
                stack.push(cmd);
            } fi
    };
};
 
 class StackCommand_Exit inherits StackCommand {
     eval(stack : Stack) : OptionalInt { let ret_val : OptionalInt in ret_val };
     out_cmd(os : IO) : IO { os };
 };