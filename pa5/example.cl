(*
    Example cool program testing as many aspects of the code generator
    as possible.
*)

class Main inherits IO {
  integer : Int <- 10 + 20;
  other : Main <- new Main;
  bool : Bool <- isvoid integer;
  bool2 : Bool <- let val:Bool, val2:Bool in val <- val2;

  main(): SELF_TYPE {{out_string("Example!\n");
   case integer of
      obj1 : String => 0;
      obj2 : Int => 1;
      obj3 : Main => 2;
   esac;
   self;
   }};
};

class A inherits Main {
  test : Int <- integer;
  integer2 : Int <- 20;
  str : String <- "TestString";
  other2 : Main <- other.main();
};
