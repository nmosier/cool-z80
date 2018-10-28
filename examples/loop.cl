class Main inherits IO {
	i1: Int;

	main(): SELF_TYPE {{
		out_string("n: ");
		i1 <- in_int();
		
		
		while 0 < i1 loop {
			out_int(i1);
			out_string("\n");
			i1 <- i1 - 1;
		} pool;
		
		self;
	}};

};