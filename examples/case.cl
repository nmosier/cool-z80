class A inherits IO {
	print(): SELF_TYPE {
		out_string("A")
	};
};

class B inherits A {
	print(): SELF_TYPE {
		out_string("B")
	};
};

class C inherits A {
	printC(): SELF_TYPE {
		out_string("C")
	};
};

class Main inherits IO {

	casetest(obj: Object): Object {
		case obj of
			int: Int => out_int(int);
			str: String => out_string(str);
			a: A => a.print();
			b: B => b.print();
			c: C => c.printC();
		esac
	};

	main(): SELF_TYPE {{
		casetest(1);
		casetest("fuck");
		casetest(new A);
		casetest(new B);
		casetest(new C);
		self;
	}};

};