(*
 * test_int.cl
 *
 * (semantically invalid) Cool program
 * for testing integers
 *)

-- Basic int tests
{
	0;
	-1;
	2147483647;
	-2147483647;
	2147483648;
	-2147483648;
	0000000;
	0000001;
	0000010;
}

-- Other int tests
{
	(* 123 is NOT an INT_CONST *)
	10*20
	(230)
	_010_
	{10;}
}
99