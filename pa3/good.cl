class TestA {
	-- testing attributes
	num: Int;
	str: String <- "This is a string.";
	num2: Int <- let num2: Int in num2 <- 2;
	
	-- testing methods
	noargs() : String {
		"no arguments method"
	};
	
	onearg(formal1: Object) : Object {
		new Object
	};
	
	multiarg(formal1: Bool, formal2: Int, formal3: String, io: IO) : IO {{
		if (formal1)
		then
			io.out_int(formal2)
		else
			io.out_string(formal3)
		fi;
	}};
	
	-- testing all expression productions
	test_expressions() : Object {{
		if (id <- true)
		then
			while not false
			loop
				{
					let i: Int <- new Int in isvoid new Bool;
					let i: Int, i2: Int <- i + i in i2 - i;
				}
			pool
		else
			case new IO of
				num : Int => ~num + num - num * num + num / num - num;
				b : Bool => ((((not b))));
				io : IO => (true < false) = (true <= false);
			esac
		fi;
		io.out_int(99999);
		io.out_string("test");
		(new TestA)@SELF_TYPE.method(arg);
		noargs();
		~int2 <- 1;
	}};
	
	test_let(io: IO): IO {{
		let i: Int <- 0 in
			let i: Int <- 1 in i + i;	-- should output '2', not '1';
	}};


};

Class BB__ inherits A {
	num: Int;
	str: String <- "This is a string.";
	
	noargs() : String {
		"no arguments method"
	};
	
	onearg(formal1: Object) : Object {
		new Object
	};
};