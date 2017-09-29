(*
 * test_comments.cl
 *
 * Cool program
 * for comments
 *)

-- test single-line comments
let Object : obj <- 0 in --comment
1+1--comment
--comment 1+1
1--2=-1
--single-line--comments--don't--nest--
"--this is not a comment"

(* now test multi-line comments *)
(* this (* comment (* is *) nested *) ! *)
--This unmatched nested comment is ignored: (*
(* unmatched: *) *)
(* this
 * is
 *
 * on multiple lines
 *)
(* --this single-line comment is ignored *)

(* now it gets a bit crazy... *)
(**())(*)***(x)_
			* ) *) )))*((( ( * *)
(*)(*)nest depth = 2(*)(*)

identifier (*
		not an identifier
*)

identifier2

(* EOF in comment: