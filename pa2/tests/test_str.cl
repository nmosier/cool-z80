(*
 * test_str.cl
 *
 * (semantically invalid) Cool program
 * for testing strings
 *)
 
"This is a valid string."
"\T\h\i\s\ \i\s\ \a\l\s\o\ \v\a\l\i\d\""
"This too \
 is valid."
"This is a \0 (zero)."
"This is not valid
(*"this isn't considered a token"*) -- "nor is this"
"This looks \b normal."
"This contains  null."
"These are""two strings"

30

-- test max allowed string length (1024 chars)
"This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!"

-- make sure escape sequences counted as one character, not two
"This string is \1\0\2\4 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!"

-- this string is too long and should generate an error
"This string is 1025! chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!"

-- this string is on two lines and therefore is too long
"This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!\
This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!This string is 1024 chars long!!"

" This gives EOF Error