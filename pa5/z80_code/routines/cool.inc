#define _string_objdata_offset 2*4
#define _string_basesize 2*4
#define _int_objdata_offset 2*3
#define _int_size 2*4
#define _obj_size_field 2*1
#define _objdata_offset 2*3

#define libmem appData

#define _keyboard_newline			$0A

#define _MyDispHL_scrap 			libmem+0 ; 5 bytes
#define _MyDispHL_scrap_end 		libmem+5 ; 0 bytes

;; keyboard
#define lastKey 					libmem+5 ; 1 byte
#define _keyboard_buffer_start 		libmem+6 ; 2 bytes, beginning of unread input
#define _keyboard_buffer_buffered 	libmem+8 ; 2 bytes, (one past) end of buffered input
#define _keyboard_buffer_input 		libmem+10; 2 bytes, (one past) end of unbuffered input
#define _keyboard_flags 			libmem+12; 1 byte
#define _keyboard_flag_alock		0
#define _keyboard_flag_shift		1	; note alock and shift are mutually exclusive

;; IO
#define IO.in_int.flags				libmem+13; 1 byte
#define IO.in_int.flags.neg			0

;; String
#define String.concat.strobj		libmem+15; 2 bytes

;; memory
#define _memory_flags 				libmem+17; 1 byte
#define _memory_init_f				0
#define _memory_begin				libmem+18; 2 bytes
#define _memory_free				libmem+20; 2 bytes
#define _memory_end					libmem+22; 2 bytes


#macro high(x)
	x >> 8
#endmacro
#macro low(x)
	x & 0xFF
#endmacro