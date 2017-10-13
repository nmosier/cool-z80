
(*
 *  Execute "coolc bad.cl" to see the error messages that the coolc parser
 *  generates
 *)

(* no error *)
class A {
};

(* error:  b is not a type identifier *)
Class b inherits A {
};

(* error:  a is not a type identifier *)
Class C inherits a {
};

(* error:  keyword inherits is misspelled *)
Class D inherts A {
};

(* error:  closing brace is missing *)
Class E inherits A {
;

-- what follows is a version of good.cl modified to produce errors

class TestA {
	-- testing error recovery for invalid attributes
	;
	num: Int
	str: String <- "This is a string.";;
	num2: Int <- let num2: Int in num2 <- 2;
	
	-- testing error recovery for invalid methods
	noargs() : String {
	};
	
	 : Object {
		new Object
	};
	
	multiarg(formal1, formal2, formal3, io) : IO {{
		if (formal1)
		then
			io.out_int(formal2)
		else
			io.out_string(formal3)
		fi;
	}};
	
	another_method() {
		1
	};
	
	-- testing let binding and block statement error recovery
	let_bindings() : Void {{
		-- valid let stmt
		let i: Int in i;
		-- invalid
		let i: Int, i : in i;	-- error
		let i: Int in i;
		let i: Int, : Int, i3: Int in i + i2 + i3;	-- error
		let i: Int in i;
		let Int, i: Int, i2: Int in i + i2 + i3;	-- error
		let i: Int in i;
		let i: in i;	-- error
		if then else fi;	-- error
	}};
	
	
	-- test that each invalid expression generates an error
	test_expressions() : Object {{
		(new Object)@object.method(arg, arg2);
		method(arg;
		if true then else false fi;
		while false loop pool;
		{ (* block cannot be empty *) };
		case Type of int: Int => int+1; esac;
		new objectID;
		isvoid isvoid;
		1 + TypeID;
		2 - ;
		1 * ;
		TypeID / 1;
		~objectID;
		1 < 2 < 3;
		1 <= 2 <= 3;
		1 = 1 = 1;
		not not Int;
		();
		-- don't need to test terminals (objectID, integer, string, etc.) => can't be errors
		-- valid expr:
		let i: Int in io.out_int(i);
	}};
	
	test_let(io: IO): IO {{
		let i: Int <- 0 in
			let i: Int <- 1 in i + i;	-- should output '2', not '1';
	}};


};

Class BB__ inherits A {
	
};