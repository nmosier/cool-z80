class Main inherits IO {
	str1: String <- "This string is long.";
	str2: String <- "Short!";
	str3: String <- "";
	
	main(): Object {{
		out_string(str1);
		out_int(str1.length());
		out_string("\n");
		out_string(str2);
		out_int(str2.length());
		out_string("\n");
		out_string(str3);
		out_int(str3.length());
		out_string("\n");
		in_string();
	}};
	
};