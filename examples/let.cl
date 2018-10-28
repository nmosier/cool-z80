class Global {
	id: Int <- ~1;
	
	is_init(): Bool {
		if id < 0 then false else true fi
	};
	
	set_id(new_id: Int): SELF_TYPE {{
		id <- new_id;
		self;
	}};
};

class Main inherits IO {
	globl1: Global <- (new Global).set_id(0);
	globl2: Global <- (new Global).set_id(1);
	
	main(): SELF_TYPE {{
		out_string("0: ");
		out_string(
			if globl1.is_init() then "Y " else "N " fi
		);
		out_string(
			if globl2.is_init() then "Y\n" else "N\n" fi
		);
		
		
		out_string("1: ");
		let globl1: Global <- new Global in {
			out_string(
				if globl1.is_init() then "Y " else "N " fi
			);
			out_string(
				if globl2.is_init() then "Y\n" else "N\n" fi
			);
		};


		out_string("2: ");
		let globl1: Global <- new Global, globl2: Global <- new Global in {
			out_string(
				if globl1.is_init() then "Y " else "N " fi
			);
			out_string(
				if globl2.is_init() then "Y\n" else "N\n" fi
			);
		};
	}};
};