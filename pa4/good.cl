class D inherits C {
    init(x: Int, y: Bool) : C { self };
};

class C inherits Main {
	a : Int;
	b : Bool;
	init(x : Int, y : Bool) : C { {
		a <- x;
		b <- y;
		self;
    } };
};

Class Main {
	main():C {
	  (new C).init(1, true)
	};
};

class Expressions {
	-- per the Cool manual, all attributes globally available within class
	attr1 : Int <- attr2;
	attr2 : Int <- attr1;
	
	-- features can have same identifiers as long as they are of different type
	feature : Int;
	feature() : Int { 1 };

};

class Case_A {
	method() : SELF_TYPE { self };
	method2() : SELF_TYPE { self };
};

class Case_B inherits Case_A {
	method() : SELF_TYPE { new SELF_TYPE };
};

class Case_C inherits Case_B {
};

class CheckCase {
	method(obj : Case_B) : Case_B {
		case obj of
			obj_b : Case_B => obj_b.method();
			obj_c : Case_C => obj_c.method();
		esac
	};
	
	method2(obj : Case_A) : Case_B {
		case obj of
			obj_b : Case_B => obj_b.method2();
			obj_c : Case_C => obj_c.method2();
		esac
	};
};

class CheckLet {
	integer : Object;
	test_int(integer : Int) : Int {
		let integer : Object, integer : String, integer : Int in integer
	};


};