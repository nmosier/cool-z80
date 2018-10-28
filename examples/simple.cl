class Printer inherits IO {
	name: String <- "printer";

	set_name(str: String): SELF_TYPE {{
		name <- str;
		self;
	}};

	print(): SELF_TYPE {{
		out_string(name);
	}};
};

class Main inherits IO {
	i1 : Int;
	i2 : Int;

	test_lt(i1 : Int, i2 : Int): SELF_TYPE {{
		(* test < *)
		out_int(i1);
		out_string(if i1 < i2 then "<" else ">=" fi);
		out_int(i2);
		out_string("\n");
	}};
	
	test_le(i1: Int, i2: Int): SELF_TYPE {{
		(* test >= *)
		out_int(i1);
		out_string(if i1 <= i2 then "<=" else ">" fi);
		out_int(i2);
		out_string("\n");
	}};
	
	test_eq(i1: Int, i2: Int): SELF_TYPE {{
		(* test = *)
		out_int(i1);
		out_string(if i1 = i2 then "=" else "!=" fi);
		out_int(i2);
		out_string("\n");
	}};
	
	test_neg(i1: Int, i2: Int): SELF_TYPE {{
		out_string("neg: ");
		out_int(~i1);
		out_int(~i2);
		out_string("\n");
	}};
	
	test_not(i1: Int, i2: Int): SELF_TYPE {{
		out_int(i1);
		out_string(if not (i1 = i2) then "!=" else "=" fi);
		out_int(i2);
		out_string("\n");
	}};
	
	test_isvoid(obj: Object): SELF_TYPE {{
		out_string("object ");
		out_string(if isvoid obj then "void\n" else "not void\n" fi);
	}};
	
	test_eqobj(obj1: Printer, obj2: Printer): SELF_TYPE {{
		obj1.print();
		out_string(if obj1 = obj2 then "=" else "!=" fi);
		obj2.print();
		out_string("\n");
	}};
	
	obj1: Printer <- (new Printer).set_name("printer1");
	obj2: Printer <- (new Printer).set_name("printer2");

	main(): SELF_TYPE {{
		out_string("i1: ");
		i1 <- in_int();
		out_string("i2: ");
		i2 <- in_int();
		test_lt(i1, i2);
		test_le(i1, i2);
		test_eq(i1, i2);
		test_neg(i1, i2);
		in_string(); -- wait for input
		test_not(i1, i2);
		test_isvoid(obj1);
		test_eqobj(obj1, obj2);
		in_string(); -- wait for input
		self;
	}};
};