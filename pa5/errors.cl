(* Nicholas Mosier 2017
 *
 *)

class Main inherits IO {
  -- case_abort
  other : Main;
  test_case_abort():SELF_TYPE {{
    case other of
      m : Main => out_string("Main\n");
      io : IO => out_string("IO.\n");
      obj : Object => out_string("Object.\n");
    esac;
    self;
  }};
  
  -- case_abort2
  test_case_abort2():SELF_TYPE {{
    case (new SELF_TYPE) of
      a:A => out_string("a\n");
      b:B => out_string("b\n");
      c:C => out_string("c\n");
      i:Int => out_string("i\n");
    esac;
    self;
  }};
  
  -- dispatch_abort
  test_dispatch_abort():SELF_TYPE {{
    other.main();
    self;
  }};
  
  main() : SELF_TYPE {{
    -- test_case_abort();
    -- test_case_abort2();
    -- test_dispatch_abort();
    
    self;
  }};
};

class A inherits Main {};
class B inherits Main {};
class C inherits A {};