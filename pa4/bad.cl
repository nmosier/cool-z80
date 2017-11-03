class C {
	a : Int;
	b : Bool;
	init(x : Int, y : Bool) : C { {
		a <- x;
		b <- y;
		self;
    } };
};

-- no Main class defined
Class NotMain {
	main() : C { {
        (new C).init(1,1);
        (new C).init(1,true,3);
        (new C).iinit(1,true);
        (new C);
	} };
};

class C {
	a : Int;
	b : Bool;
	init(x : Int, y : Bool) : C { {
		a <- x;
		b <- y;
		self;
    } };
};

Class OtherClass {
	main():C {
	  (new C).init(1, true)
	};
};

-- inheritance of nonexistent class
class B inherits M {
	a: Int;
};

class C {
	a:Int;
};

-- illegal inheritance of non-inheritable object
class D inherits Int {
	a: Int;
};

class E inherits C {
	method1(formal1: Int) : Int {0};
	
	num : Int;
};

-- Illegal redefinition of method within same class
-- Illegal overloading of inherited attribute
class F inherits E {
	method1(formal1: Int) : Int {1};
	method1(formal2: Object) : Int {2};
	
	num : String;
};

-- Illegal inheritance cycle
class G inherits H {
};

class H inherits G {
};

-- Illegal self-inheritance
class I inherits I {};

(* Illegal overloading of methods
	- different return type
	- same # of formals, different types
	- different # of formals
*)
class J {
	method(formal: J) : J {self};
	method2(formal: SELF_TYPE) : SELF_TYPE {self};
};

class H inherits J {
	method(formal_new: H) : H {self};
	method2(formal: SELF_TYPE, formal2: SELF_TYPE) : SELF_TYPE {self};
};

(* error catching of undefined identifiers, methods, and types *)
class Undefined {
	undefined() : SELF_TYPE {{
		let var1 : UndefinedType <- self.undefinedDispatch() in
			var2 <- 1;
	}};
};

(* test invalid attributes & methods *)
class InvalidAttributes {
	attr1: NotAType <- new SELF_TYPE;
	attr2: Int <- 2 + attr1;
	
	method1(var1: BadType) : WorseType { 1 };
	method2() : SELF_TYPE {{
		(new SELF_TYPE).method1(1);	-- shouldn't generate any errors
	}};
};


(* now testing typechecking in expressions *)

class Parent {};
class Child inherits Parent {};

class Expressions {
	-- ASSIGN
	assign(var : SELF_TYPE, child: Child) : SELF_TYPE {{
		var <- 1; -- error: not case that Int ≤ SELF_TYPE
		child <- new Parent; -- error: not case that Parent ≤ Child
	}};
	
	-- STATIC DISPATCH
	static_dispatch(var : SELF_TYPE) : SELF_TYPE {{
		var@InvalidDispatch.method(1, 2, 3, 4);
	}};
	
	-- CASE
	cases(var : SELF_TYPE) : SELF_TYPE {{
		case var of
			invalid_type : G => invalid_type;
			valid_id : Int => 1;
			invalid_type_2 : SELF_TYPE => invalid_type_2; -- identifier cannot be of type SELF_TYPE
		esac;
	}};

	-- NEW
	invalid_new(var : SELF_TYPE) : SELF_TYPE {{
		var <- new None;
		var <- new I;
	}};
};

class CheckLTE {
	-- typecheck of CheckLTE ≤ SELF_TYPE fails
	method() : SELF_TYPE { new CheckLTE };
};

class InvalidDispatch_A {
	method() : SELF_TYPE { self };
};
class InvalidDispatch_B inherits InvalidDispatch_A {
};

class InvalidDispatch {
	invalid_dispatch() : InvalidDispatch_A {
		((new InvalidDispatch_A).method())@InvalidDispatch_B.method()
	};
};