#include <iostream>
#include <string>
#include <set>
#include "register.h"
#include "stringtab.h"
#include "cgen_supp.h"
#include "emit.h"

//////////////////////////////////////////////////////////////////////////////
//
//  emit_* procedures
//
//  emit_X  writes code for operation "X" to the output stream.
//  There is an emit_X for each opcode X, as well as emit_ functions
//  for generating names according to the naming conventions (see emit.h)
//  and calls to support functions defined in the trap handler.
//            
//////////////////////////////////////////////////////////////////////////////
namespace cool {

extern Symbol
    *arg,
    *arg2,
    *Bool,
    *concat,
    *cool_abort,
    *copy,
    *Int,
    *in_int,
    *in_string,
    *IO,
    *isProto,
    *length,
    *Main,
    *main_meth,
    *No_class,
    *No_type,
    *Object,
    *out_int,
    *out_string,
    *prim_slot,
    *self,
    *SELF_TYPE,
    *String,
    *str_field,
    *substr,
    *type_name,
    *val;

void emit_include(const std::string& filename, std::ostream& os) {
	os << INCLUDE << "\"" << filename << "\"" << std::endl;
}

void emit_load(const RegisterValue& dst, const LabelValue& src, std::ostream& os) {
	assert (dst.size() == src.size());
	os << LD << dst << "," << src << std::endl;
}
void emit_load(const RegisterPointer& dst, const LabelValue& src, std::ostream& os) {
	os << LD << dst << "," << src << std::endl;
}
void emit_load(const RegisterValue& dst, const RegisterValue& src, std::ostream& os) {
	assert (dst.size() == src.size());
   assert (!src.reg() || *src.reg() != SP);

	if (dst.size() == 1) {
		os << LD << dst << "," << src << std::endl;
	} else if (dst.size() == 2) {
		const Register16 *dst_reg = (const Register16 *) dst.reg(), *src_reg = (const Register16 *) src.reg();
		if (*dst_reg == SP) {
			assert (*src_reg == rHL || *src_reg == rIX || *src_reg == rIY);
			os << LD << *dst_reg << "," << *src_reg << std::endl;
		} else {
			os << LD << dst_reg->high() << "," << src_reg->high() << std::endl;
			os << LD << dst_reg->low() << "," << src_reg->low() << std::endl;
		}
	} else {
		std::cerr << "register values of mismatching sizes" << std::endl;
		throw "register values of mismatching sizes";
	}
}
void emit_load(const MemoryValue& dst, const RegisterValue& src, std::ostream& os) {
	if (dst.loc().kind() == MemoryLocation::Kind::ABS) {
		if (src.size() == 1) {
			assert (src.reg() && *src.reg() == ACC);
		}
		os << LD << dst << "," << src << std::endl;
	} else if (dst.loc().kind() == MemoryLocation::Kind::PTR) {
			const RegisterPointer& ptr = (const RegisterPointer&) dst.loc();
			assert (ptr.reg() != *src.reg()); // this allows operations such as ld (hl),h ... but maybe that's ok
			if (ptr.reg() != rHL) {
				assert (*src.reg() == ACC);		 // ld (bc),a and ld (de),a only allowed
			}
			if (src.size() == 1) {
				os << LD << dst << "," << src << std::endl;
			} else {
				const Register16& src_reg = (const Register16&) *src.reg();
				os << LD << dst << "," << src_reg.low() << std::endl;
				os << INC << *dst.reg() << std::endl;
				os << LD << dst << "," << src_reg.high() << std::endl;
				os << DEC << *dst.reg() << std::endl;
			}
	} else {
			assert (dst.loc().kind() == MemoryLocation::Kind::PTR_OFF);
			const RegisterPointerOffset& ptr_off = (const RegisterPointerOffset&) dst.loc();
			assert (ptr_off.reg() != *src.reg());
			if (src.size() == 1) {
				os << LD << dst << "," << src << std::endl;
			} else {
				const Register16& src_reg = (const Register16&) *src.reg();
				os << LD << dst << "," << src_reg.low() << std::endl;				
				os << LD << dst[1] << "," << src_reg.high() << std::endl;
			}
	}	
}

void emit_load(const RegisterValue& dst, const MemoryValue& src, std::ostream& os) {
	if (src.loc().kind() == MemoryLocation::Kind::ABS) {
		if (dst.size() == 1) {
			assert (*src.reg() == ACC);
		}
		os << LD << dst << "," << src << std::endl;
	} else if (src.loc().kind() == MemoryLocation::Kind::PTR) {
		const RegisterPointer& src_ptr = (const RegisterPointer&) src.loc();
		const Register16& src_ptr_reg = (const Register16&) src_ptr.reg();
		if (dst.size() == 1) {
			assert (*dst.reg() != src_ptr_reg.high() && *dst.reg() != src_ptr_reg.low());
			os << LD << dst << "," << src << std::endl;
		} else {
			const Register16& dst_reg = (const Register16&) *dst.reg();
			assert (dst.size() == 2 && *dst.reg() != src_ptr_reg);
			os << LD << dst_reg.low() << "," << src << std::endl;
			os << INC << src_ptr_reg << std::endl;
			os << LD << dst_reg.high() << "," << src << std::endl;
			os << DEC << src_ptr_reg << std::endl;
		}
	} else if (src.loc().kind() == MemoryLocation::Kind::PTR_OFF) {
		const RegisterPointerOffset& loc = (const RegisterPointerOffset&) src.loc();
		const Register16& dst_reg = (const Register16&) *dst.reg();
		if (dst.size() == 1) {
			os << LD << dst << "," << src << std::endl;
		} else if (dst.size() == 2) {
			os << LD << dst_reg.low() << "," << MemoryValue(loc) << std::endl;
			os << LD << dst_reg.high() << "," << MemoryValue(loc[1]) << std::endl;
		} else {
			std::cerr << "invalid size of dst" << std::endl;
			throw "invalid size of dst";
		}
	}	else {
		std::cerr << "unknown type of memory location" << std::endl;
		throw "unknown type of memory location";
	}
}


void emit_neg(const Register& dst, std::ostream& s) {
	if (dst.size() == 1) {
		emit_load(ACC, dst, s);
		s << NEG << std::endl;
		emit_load(dst, ACC, s);
	} else if (dst.size() == 2) {
		const Register16& reg = (const Register16&) dst;
		emit_load(ACC, reg.low(), s);
		s << CPL << std::endl;
		emit_load(reg.low(), ACC, s);
		emit_load(ACC, reg.high(), s);
		s << CPL << std::endl;
		emit_load(reg.high(), ACC, s);
		s << INC << dst << std::endl;	
	} else {
		assert (false);
	}
}

void emit_cpl(const Register& dst, std::ostream& s) {
	if (dst.size() == 1) {
		emit_load(ACC, dst, s);
		s << CPL << std::endl;
		emit_load(dst, ACC, s);
	} else if (dst.size() == 2) {
		const Register16& dst_ = (const Register16&) dst;
		emit_load(ACC, dst_.low(), s);
		s << CPL << std::endl;
		emit_load(dst_.low(), ACC, s);
		emit_load(ACC, dst_.high(), s);
		s << CPL << std::endl;
		emit_load(dst_.high(), ACC, s);
	} else {
		assert (false);
	}
}

void emit_add(const RegisterValue& dst, const RegisterValue& src, std::ostream& s) {
	assert (dst.reg() && (*dst.reg() == ACC || *dst.reg() == rIX || 
                         *dst.reg() == rIY || *dst.reg() == rHL));
	s << ADD << dst << "," << src << std::endl;
}

void emit_add(const RegisterValue& dst, const Immediate8& src, std::ostream& s) {
	assert (dst.reg() && *dst.reg() == ACC);
	s << ADD << dst << "," << src << std::endl;
}

void emit_add(const RegisterValue& dst, const MemoryValue& src, std::ostream& s) {
	assert (dst.reg() && *dst.reg() == ACC);
	if (src.loc().kind() == MemoryLocation::Kind::PTR) assert (src.reg() && *src.reg() == rHL);
	else if (src.loc().kind() == MemoryLocation::Kind::PTR_OFF) ;
	else {
		std::cerr << "can only add (hl) or (ix+*) or (iy+*) to ACC" << std::endl;
		throw "can only add (hl) or (ix+*) or (iy+*) to ACC";
	}
	s << ADD << dst << "," << src << std::endl;
}

void emit_adc(const RegisterValue& dst, const RegisterValue& src, std::ostream& s) {
	assert (dst.reg() && (*dst.reg() == ACC || *dst.reg() == rIX || *dst.reg() == rHL));
	s << ADC << dst << "," << src << std::endl;
}

void emit_adc(const RegisterValue& dst, const Immediate8& src, std::ostream& s) {
	assert (dst.reg() && *dst.reg() == ACC);
	s << ADC << dst << "," << src << std::endl;
}

void emit_adc(const RegisterValue& dst, const MemoryValue& src, std::ostream& s) {
	assert (dst.reg() && *dst.reg() == ACC);
	if (src.loc().kind() == MemoryLocation::Kind::PTR) assert (src.reg() && *src.reg() == rHL);
	else if (src.loc().kind() == MemoryLocation::Kind::PTR_OFF) ;
	else {
		std::cerr << "can only add (hl) or (ix+*) or (iy+*) to ACC" << std::endl;
		throw "can only add (hl) or (ix+*) or (iy+*) to ACC";
	}
	s << ADC << dst << "," << src << std::endl;
}



bool compatible_SUB(const Value& src) {
	if (src.kind() == Value::Kind::NONE)
		return false;
	if (src.kind() == Value::Kind::MEM) {
		const MemoryLocation& loc = ((const MemoryValue&) src).loc();
		if (loc.kind() == MemoryLocation::Kind::NONE)
			return false;
		else if (loc.kind() == MemoryLocation::Kind::PTR_OFF)
			return ((const RegisterPointerOffset&) loc).reg() == rIX;
		else
			return true;
	}
	return src.size() == 1;
}

void emit_sub(const Value& src, std::ostream& s) {
	assert (compatible_SUB(src));
	s << SUB << src << std::endl;
}


void emit_inc(const Register& dst, std::ostream& s) {
   s << INC << dst << std::endl;
}

void emit_dec(const Register& dst, std::ostream& s) {
	s << DEC << dst << std::endl;
}


void emit_sla(const Register8& dst, std::ostream& s) {
	s << SLA << dst << std::endl;
}


void emit_sra(const Register8& dst, std::ostream& s) {
	s << SRA << dst << std::endl;
}


void emit_srl(const Register8& dst, std::ostream& s) {
	s << SRL << dst << std::endl;
}


void emit_jr(const AbsoluteAddress& loc, Flag flag, std::ostream& s) {
	s << JR;
	if (flag)
		s << flag << ",";
	s << loc << std::endl;
}
void emit_jr(int label_number, Flag flag, std::ostream& s) {
	const AbsoluteAddress label_addr(std::string("label") + std::to_string(label_number));
	emit_jr(label_addr, flag, s);
}


bool compatible_JP(const MemoryLocation& dst) {
	switch (dst.kind()) {
	case MemoryLocation::PTR:
		return ((const RegisterPointer&) dst).reg() == rHL;
	case MemoryLocation::ABS:
		return true;
	case MemoryLocation::NONE:
		return false;
	case MemoryLocation::PTR_OFF:
		return true;
	default:
		return false;
	}
}

void emit_jp(const MemoryLocation& loc, Flag flag, std::ostream& s) {
	assert (compatible_JP(loc));
	s << JP;
	if (flag)
		s << flag << ",";
	if (loc.kind() == MemoryLocation::Kind::PTR)
		s << "(" << loc << ")" << std::endl;
	else
		s << loc << std::endl;
}
void emit_jp(int label, Flag flag, std::ostream& s) {
	char label_str[100];
	sprintf(label_str, "label%d", label);
	AbsoluteAddress loc(label_str);
	emit_jp(loc, flag, s);
}


void emit_return(Flag flag, std::ostream& s) {
	s << RET;
	if (flag)
		s << flag;
	s << std::endl;
}


void emit_call(const AbsoluteAddress& addr, Flag flag, std::ostream& s) {
	s << CALL;
	if (flag)
		s << flag << ",";
	s << addr << std::endl;
}


void emit_copy(std::ostream& s) {
	AbsoluteAddress addr("Object.copy");
	emit_call(addr, NULL, s);
}

void emit_gc_assign(std::ostream& s) {
	AbsoluteAddress addr("_GenGC_Assign");
	emit_call(addr, NULL, s);
}

void emit_equality_test(std::ostream& s) {
	AbsoluteAddress addr("equality_test");
	emit_call(addr, NULL, s);
}

void emit_case_abort(std::ostream& s) {
	AbsoluteAddress addr("_case_abort");
	emit_call(addr, NULL, s);
}

void emit_case_abort2(std::ostream& s) {
	AbsoluteAddress addr("_case_abort2");
	emit_call(addr, NULL, s);
}

void emit_dispatch_abort(std::ostream& s) {
	AbsoluteAddress addr("_dispatch_abort");
	emit_call(addr, NULL, s);
}


std::string label_ref(int l) {
	char tmp[100];
	sprintf(tmp, "label%d", l);
	return std::string(tmp);
}

void emit_label_ref(int l, std::ostream &s)
{ s << "label" << l; }

void emit_label_def(int l, std::ostream &s)
{
  emit_label_ref(l,s);
  s << ":" << std::endl;
}

// Push a register on the stack. The stack grows towards smaller addresses.
//
bool compatible_PUSH(const Register& src) {
	return src.size() == 2;
}
void emit_push(const Register& src, std::ostream& s) {
	assert (compatible_PUSH(src));
	s << PUSH << src << std::endl;
}

bool compatible_POP(const Register& dst) {
	return dst.size() == 2;
}
// Pop word from stack into register
void emit_pop(const Register& dst, std::ostream& s) {
  s << POP << dst << std::endl;
}


void emit_cp(const Register8& src, std::ostream& s) {
  s << CP << src << std::endl;
}
void emit_cp(const RegisterPointer& src, std::ostream& s) {
  assert (src.reg() == rHL);
  s << CP << MemoryValue(src) << std::endl;
}

void emit_or(const Register8& src, std::ostream& s) {
  s << OR << src << std::endl;
}
void emit_or(const RegisterPointer& src, std::ostream& s) {
  assert (src.reg() == rHL);
  s << OR << src << std::endl;
}


///////////


// Fetch the integer value in an Int object.
//
void emit_fetch_int(const RegisterValue& dst, const MemoryValue& src, std::ostream& s) {
	assert (src.size() == 2 || src.size() == 0);
	assert (dst.size() == 2);
	if (src.loc().kind() == MemoryLocation::Kind::NONE) {
		assert (src.loc().kind() != MemoryLocation::Kind::NONE);
	} else if (src.loc().kind() == MemoryLocation::Kind::ABS) {
		// can load in one LD instruction
		const AbsoluteAddress& addr = (const AbsoluteAddress&) src.loc();
		emit_load(dst, addr[DEFAULT_OBJFIELDS], s);
	} else if (src.loc().kind() == MemoryLocation::Kind::PTR) {
		// get used registers
		const Register16& dst_reg = (const Register16&) *dst.reg();
		const Register16& src_reg = (const Register16&) *src.reg();
		int16_t offset = DEFAULT_OBJFIELDS * WORD_SIZE;
		
		assert (src_reg == rHL && dst_reg != src_reg); // make sure not loading to same register
		
		// find scrap register
		std::set<const Register16 *> regs16;
		regs16.insert(&rHL); regs16.insert(&rDE); regs16.insert(&rBC);
		regs16.erase(&dst_reg); regs16.erase(&src_reg);
		const Register16& scrap_reg = **regs16.begin();
		
		// can load in two 1-byte LD instructions
		emit_push(scrap_reg, s);
		emit_load(RegisterValue(scrap_reg), Immediate16(offset), s);
		emit_add(RegisterValue(src_reg), Register(scrap_reg), s);
		
		emit_load(dst_reg.low(), src, s);
		emit_inc(src_reg, s);
		emit_load(dst_reg.high(), src, s);
		
		s << SCF << std::endl;
		s << SBC << src_reg << "," << scrap_reg << std::endl;
		emit_pop(scrap_reg, s);
	} else if (src.loc().kind() == MemoryLocation::Kind::PTR_OFF) {
		const RegisterPointerOffset& src_loc = (const RegisterPointerOffset&) src.loc();
		assert (DEFAULT_OBJFIELDS >= -128 && DEFAULT_OBJFIELDS+1 < 128);
		assert (dst.reg() && *dst.reg() != ((const RegisterPointerOffset&) src.loc()).reg());
		const Register16& dst_reg = (const Register16&) *dst.reg();
		emit_load(dst_reg.low(), src_loc[DEFAULT_OBJFIELDS], s);
		emit_load(dst_reg.high(), src_loc[DEFAULT_OBJFIELDS+1], s);
	} else {
		assert (false);
	}
}

// Update the integer value in an int object.
//
void emit_store_int(const RegisterValue& src, const MemoryValue& dst, std::ostream& s) {
	assert (src.size() == 2);
	assert (dst.size() == 2 || dst.size() == 0);
	
	if (dst.loc().kind() == MemoryLocation::Kind::NONE) {
		assert (false);
	} else if (dst.loc().kind() == MemoryLocation::Kind::ABS) {
		// load 2 bytes in one instr.
		const AbsoluteAddress& addr = (const AbsoluteAddress&) dst.loc();
		emit_load(addr[DEFAULT_OBJFIELDS], src, s);
	} else if (dst.loc().kind() == MemoryLocation::Kind::PTR) {
		// get used registers
		const Register16& dst_reg = (const Register16&) *dst.reg();
		const Register16& src_reg = (const Register16&) *src.reg();
		int16_t offset = DEFAULT_OBJFIELDS * WORD_SIZE;
	
		assert (dst_reg == rHL && src_reg != dst_reg);
		
		// find scrap register
		std::set<const Register16 *> regs16;
		regs16.insert(&rHL); regs16.insert(&rDE); regs16.insert(&rBC);
		regs16.erase(&dst_reg); regs16.erase(&src_reg);
		const Register16& scrap_reg = **regs16.begin();
		
		emit_push(scrap_reg, s);
		emit_load(RegisterValue(scrap_reg), Immediate16(offset), s);
		emit_add(RegisterValue(dst_reg), Register(scrap_reg), s);
			
		emit_load(dst, src_reg.low(), s);
		emit_inc(dst_reg, s);
		emit_load(dst, src_reg.high(), s);
		
		s << SCF << std::endl;
		s << SBC << dst_reg << "," << scrap_reg << std::endl;
		emit_pop(scrap_reg, s);
	} else if (dst.loc().kind() == MemoryLocation::Kind::PTR_OFF) {
		const RegisterPointerOffset& ptr_off = (const RegisterPointerOffset&) dst.loc();
		assert (DEFAULT_OBJFIELDS >= -128 && DEFAULT_OBJFIELDS+1 < 128);
		assert (*src.reg() != ((const RegisterPointer&) dst.loc()).reg());
		const Register16& src_reg = (const Register16&) *src.reg();
		emit_load(ptr_off[DEFAULT_OBJFIELDS], src_reg.low(), s);
		emit_load(ptr_off[DEFAULT_OBJFIELDS+1], src_reg.high(), s);
	} else {
		assert (false);
	}
}

// Fetch the bool value in a Bool object.
void emit_fetch_bool(const RegisterValue& dst, const MemoryValue& src, std::ostream& s) {
	assert (dst.size() == 2);
	assert (src.size() == 2 || src.size() == 0);
	if (src.loc().kind() == MemoryLocation::Kind::NONE) {
		assert (false);
	} else if (src.loc().kind() == MemoryLocation::Kind::ABS) {
		const AbsoluteAddress& src_addr = (const AbsoluteAddress&) src.loc();
		emit_load(dst, src_addr[DEFAULT_OBJFIELDS], s);
	} else if (src.loc().kind() == MemoryLocation::Kind::PTR) {
		// get used registers
		const Register16& dst_reg = (const Register16&) *dst.reg();
		const Register16& src_reg = (const Register16&) *src.reg();
		int16_t offset = DEFAULT_OBJFIELDS * WORD_SIZE;
	
		// const RegisterPointer& src_loc = (const RegisterPointer&) src.loc();
		assert (src_reg == rHL && dst_reg != src_reg);
		
		// find scrap register
		std::set<const Register16 *> regs16;
		regs16.insert(&rHL); regs16.insert(&rDE); regs16.insert(&rBC);
		regs16.erase(&dst_reg); regs16.erase(&src_reg);
		const Register16& scrap_reg = **regs16.begin();
		
		emit_push(scrap_reg, s);
		emit_load(RegisterValue(scrap_reg), Immediate16(offset), s);
		emit_add(RegisterValue(src_reg), Register(scrap_reg), s);

		/* for (int i = 0; i < DEFAULT_OBJFIELDS * WORD_SIZE; ++i) {
			emit_inc(*src.reg(), s);
		} */
	
		emit_load(dst_reg.low(), src, s);
		emit_inc(src_reg, s);
		emit_load(dst_reg.high(), src, s);

//		emit_load(dst, src, s);
// 		emit_add(src, -DEFAULT_OBJFIELDS, s);

		/* for (int i = 0; i < DEFAULT_OBJFIELDS * WORD_SIZE; ++i) {
			emit_dec(*src.reg(), s);
		}	 */
		
		s << SCF << std::endl;
		s << SBC << src_reg << "," << scrap_reg << std::endl;
		emit_pop(scrap_reg, s);
	} else if (src.loc().kind() == MemoryLocation::Kind::PTR_OFF) {
		const RegisterPointerOffset& src_loc = (const RegisterPointerOffset&) src.loc();
		assert (DEFAULT_OBJFIELDS >= -128 && DEFAULT_OBJFIELDS < 128);
		assert (*dst.reg() != src_loc.reg().high() && *dst.reg() != src_loc.reg().low());
		emit_load(dst, src_loc[DEFAULT_OBJFIELDS], s);
	} else {
		assert (false);
	}
}

// Update the bool value in a Bool object.
void emit_store_bool(const RegisterValue& src, const MemoryValue& dst, std::ostream& s) {
	assert (src.size() == 1);
	assert (dst.size() == 2 || dst.size() == 0);
	if (dst.loc().kind() == MemoryLocation::Kind::NONE) {
		assert (false);
	} else if (dst.loc().kind() == MemoryLocation::Kind::ABS) {
		emit_load(dst[DEFAULT_OBJFIELDS], src, s);
	} else if (dst.loc().kind() == MemoryLocation::Kind::PTR) {
		// get used registers
		const Register16& dst_reg = (const Register16&) *dst.reg();
		const Register16& src_reg = (const Register16&) *src.reg();
		int16_t offset = DEFAULT_OBJFIELDS * WORD_SIZE;
	
		assert (dst_reg == rHL && src_reg != dst_reg);
		
		/* for (int i = 0; i < DEFAULT_OBJFIELDS * WORD_SIZE; ++i) {
			emit_inc(*dst.reg(), s);
		} */
		
		// find scrap register
		std::set<const Register16 *> regs16;
		regs16.insert(&rHL); regs16.insert(&rDE); regs16.insert(&rBC);
		regs16.erase(&dst_reg); regs16.erase(&src_reg);
		const Register16& scrap_reg = **regs16.begin();
		
		emit_push(scrap_reg, s);
		emit_load(RegisterValue(scrap_reg), Immediate16(offset), s);
		emit_add(RegisterValue(dst_reg), Register(scrap_reg), s);
			
		emit_load(dst, src_reg.low(), s);
		emit_inc(dst_reg, s);
		emit_load(dst, src_reg.high(), s);
		
		s << SCF << std::endl;
		s << SBC << dst_reg << "," << scrap_reg << std::endl;
		emit_pop(scrap_reg, s);
		
		//emit_load(dst, src, s);
		
		/* for (int i = 0; i < DEFAULT_OBJFIELDS * WORD_SIZE; ++i) {
			emit_dec(*dst.reg(), s);
		} */
	} else if (dst.loc().kind() == MemoryLocation::Kind::PTR_OFF) {
		const RegisterPointer& dst_loc = (const RegisterPointer&) dst.loc();
		assert (DEFAULT_OBJFIELDS >= -128 && DEFAULT_OBJFIELDS < 128);
		assert (src.reg() && *src.reg() != dst_loc.reg().high() && *src.reg() != dst_loc.reg().low());
		emit_load(dst[DEFAULT_OBJFIELDS], src, s);
	} else {
		assert (false);
	}
}

//  currently not used	
// static void emit_gc_check(const char *source, std::ostream &s)
// {
//   if (source != A1) emit_move(A1, source, s);
//   s << JAL << "_gc_check" << std::endl;
// }





void emit_protobj_ref(Symbol* sym, std::ostream& os) {
   os << sym << PROTOBJ_SUFFIX;
}

void emit_disptable_ref(Symbol* sym, std::ostream& os) {
   os << sym << DISPTAB_SUFFIX;
}

void emit_method_ref(Symbol* classname, Symbol* methodname, std::ostream& os) {
  os << classname << METHOD_SEP << methodname;
}

void emit_init_ref(Symbol* sym, std::ostream& os) {
   os << sym << CLASSINIT_SUFFIX;
}

std::string get_init_ref(Symbol* sym) {
   return std::string(sym->value()) + std::string(CLASSINIT_SUFFIX);
}

void emit_init(Symbol* classname, std::ostream& os) {
  os << CALL; emit_init_ref(classname, os); os << std::endl;
}

/**
 * Generate reference to label for String constant
 * @param os std::ostream to write generated code to
 * @param entry String constant
 * @return os
 */
const int CgenRef_strlen = 100;
std::ostream& CgenRef(std::ostream& os, const StringEntry* entry) {
  os << STRCONST_PREFIX << entry->id();
  return os;
}
std::string CgenRef(const StringEntry* entry) {
	char s[CgenRef_strlen];
	sprintf(s, "%s%lu", STRCONST_PREFIX, entry->id());
	return std::string(s);
}

/**
 * Generate reference to label for Int constant
 * @param os std::ostream to write generated code to
 * @param entry Int constant
 * @return os
 */
std::ostream& CgenRef(std::ostream& os, const Int16Entry* entry) {
  os << INTCONST_PREFIX << entry->id();
  return os;
}
std::string CgenRef(const Int16Entry* entry) {
	char s[CgenRef_strlen];
	sprintf(s, "%s%lu", INTCONST_PREFIX, entry->id());
	return std::string(s);
}

/**
 * Generate reference to label for Bool constant
 * @param os std::ostream to write generated code to
 * @param entry Bool constant
 * @return os
 */
std::ostream& CgenRef(std::ostream& os, bool entry) {
  os << BOOLCONST_PREFIX  << ((entry) ? 1 : 0);
  return os;
}
std::string CgenRef(bool entry) {
	char s[CgenRef_strlen];
	sprintf(s, "%s%d", BOOLCONST_PREFIX, (entry) ? 1 : 0);
	return std::string(s);
}


/**
 * Emit definition of string constant labeled by index in the gStringTable
 * @param os std::ostream to write generated code to
 * @param entry String constant
 * @param class_tag String class tag
 * @return os
 */
std::ostream& CgenDef(std::ostream& os, const StringEntry* entry, std::size_t class_tag) {
  const std::string& value = entry->value();
  auto length_entry = gIntTable.emplace(value.size());
  
  // Add -1 eye catcher
  os << DW << "-1" << std::endl;
  CgenRef(os, entry) << LABEL;
  os << DW << class_tag << std::endl
     << DW << (DEFAULT_OBJFIELDS+STRING_SLOTS)*WORD_SIZE + (value.size()+1) << std::endl // size
     << DW; emit_disptable_ref(String, os); os << std::endl;
  os << DW; CgenRef(os, length_entry) << std::endl;
  emit_string_constant(os, value.c_str());
  return os;
}

/**
 * Emit definition of integer constant labeled by index in the gIntTable
 * @param os std::ostream to write generated code to
 * @param entry Int constant
 * @param class_tag Int class tag
 * @return os
 */
std::ostream& CgenDef(std::ostream& os, const Int16Entry* entry, std::size_t class_tag) {
  // Add -1 eye catcher
  os << DW << "-1" << std::endl;
  CgenRef(os, entry) << LABEL;
  os << DW << class_tag << std::endl
     << DW << (DEFAULT_OBJFIELDS+INT_SLOTS)*WORD_SIZE << std::endl
     << DW; emit_disptable_ref(Int, os); os << std::endl;
  os << DW << entry->value() << std::endl;
  return os;
}

/**
 * Emit definition of a bool constant
 * @param os std::ostream to write generated code to
 * @param entry Bool constant
 * @param class_tag Bool class tag
 * @return
 */
std::ostream& CgenDef(std::ostream& os, bool entry, std::size_t class_tag) {
  // Add -1 eye catcher
  os << DW << "-1" << std::endl;
  CgenRef(os, entry) << LABEL;
  os << DW << class_tag << std::endl
     << DW << (DEFAULT_OBJFIELDS+BOOL_SLOTS)*WORD_SIZE << std::endl
     << DW; emit_disptable_ref(Bool, os); os << std::endl;
  os << DW << ((entry) ? 1 : 0) << std::endl;
  return os;
}



} // namespace cool
