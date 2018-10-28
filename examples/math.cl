class Main inherits IO {
	i1 : Int;
	i2 : Int;

	divtest(i1 : Int, i2 : Int): SELF_TYPE {{
		out_int(i1);
		out_string("/");
		out_int(i2);
		out_string("=");
		out_int(i1 / i2);
		out_string("\n");
	}};
	
	multest(i1 : Int, i2 : Int): SELF_TYPE {{
		out_int(i1);
		out_string("*");
		out_int(i2);
		out_string("=");
		out_int(i1 * i2);
		out_string("\n");
	}};

	main(): SELF_TYPE {{
		out_string("i1: ");
		i1 <- in_int();
		out_string("i2: ");
		i2 <- in_int();
		divtest(i1, i2);
	}};
};