#define _string_objdata_offset 2*4
#define _string_basesize 2*4
#define _int_objdata_offset 2*3
#define _int_size 2*4
#define _obj_size_field 2*1
#define _objdata_offset 2*3

#define _keyboard_newline $FF


#macro high(x)
	x >> 8
#endmacro
#macro low(x)
	x & 0xFF
#endmacro