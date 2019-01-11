(*  Nicholas Mosier 2017
    Example cool program testing as many aspects of the code generator
    as possible.
*)

class Main inherits IO {

  integer : Int <- 10 + 20;
  other : Main;
  other_a : A;
  bool : Bool <- isvoid integer;
  bool2 : Bool <- let val:Bool, val2:Bool in val <- val2;
  
  test_let() : SELF_TYPE {{
    out_string("testing let...\n");
    out_string("\tinteger (before): "); out_int(integer); out_string("\n");
    integer <- (let integer:Int <- integer*2, integer:Int <- integer*2, integer:Int <- integer*2 in
      integer);
    out_string("\integer (after): "); out_int(integer); out_string("\n");
    self;
  }};
  
  
  i1:Int <- 0;
  i2:Int <- i1;
  i3:Int <- i1+i2;
  i4:Int <- i2+i3;
  i5:Int <- i3+i4;
  
  print_5i(i1:Int, i2:Int, i3:Int, i4:Int, i5:Int):Int {{
    out_int(i1); out_int(i2); out_int(i3); out_int(i4); out_int(i5); i1;
  }};
  
  reset_int():SELF_TYPE {{ integer <- 0; self; }};
  
  test_eval_orders() : SELF_TYPE {{
    out_string("ints i1–i5: "); print_5i(i1,i2,i3,i4,i5); out_string("\n");
    out_string("testing evaluation order in dispatch: ");
    (new SELF_TYPE).reset_int().print_5i(i2 <- i1, i3 <- i2, i4 <- i3, i5 <- i4, i1 <- i5);
    out_string("\n");
    out_string("testing evaluation order in binary ops: ");
    i1 <- 0;
    out_int( (i1 <- i1+1) * (i1) );
    self;
  }};
  
  obj : Object;
  obj2 : Object;
  
  test_void() : SELF_TYPE {{
    if (isvoid obj)
      then out_string("obj is void.\n")
      else out_string("obj is not void.\n")
    fi;
    if ( isvoid ( while false loop self pool ) )
      then out_string("while loop returns void.\n")
      else out_string("while loop returns non-void.\n")
    fi;
    if ( obj = new Object )
      then out_string("new Object = void.\n")
      else out_string("new Objetc ≠ void.\n")
    fi;
    if ( obj = obj2 )
      then out_string("obj and obj2 both void.\n")
      else out_string("either obj or obj2 not void.\n")
    fi;
    self;
  }};

  main(): SELF_TYPE {{out_string("Example!\n");
   case integer of
      obj1 : String => 0;
      obj2 : Int => 1;
      obj3 : Main => 2;
   esac;
   case new SELF_TYPE of
     a : A => out_string("SELF_TYPE = A\n");
     b : Main => out_string("SELF_TYPE = Main\n");
   esac;
   
   -- run test methods
   test_let();
   test_eval_orders();
   test_void();
   
   self;
   }};
};

class A inherits Main {
  test : Int <- integer;
  integer2 : Int <- 20;
  str : String <- "TestString";
  other2 : Main <- other.main();
};
