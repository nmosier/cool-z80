/*
Copyright (c) 1995,1996 The Regents of the University of California.
All rights reserved.

Permission to use, copy, modify, and distribute this software for any
purpose, without fee, and without written agreement is hereby granted,
provided that the above copyright notice and the following two
paragraphs appear in all copies of this software.

IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

Copyright 2017 Michael Linderman.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef __EMIT_H
#define __EMIT_H

#include <fstream>
#include <string>
#include "register.h"
#include "stringtab.h"


///////////////////////////////////////////////////////////////////////
//
//  Assembly Code Naming Conventions:
//
//     Dispatch table            <classname>_dispTab
//     Method entry point        <classname>.<method>
//     Class init code           <classname>_init
//     Abort method entry        <classname>.<method>.Abort
//     Prototype object          <classname>_protObj
//     Integer constant          int_const<Symbol>
//     String constant           str_const<Symbol>
//
///////////////////////////////////////////////////////////////////////

#define WORD_SIZE    2
#define BYTE_SIZE    1
#define LOG_WORD_SIZE 2     // For logical shifts

// File names & paths
#define DISPTAB_PATH         "disptab.z80"
#define LIB_PATH             "z80_code/routines"

// Global names
#define CLASSNAMETAB         "class_nameTab"
#define CLASSOBJTAB          "class_objTab"
#define INTTAG               "_int_tag"
#define BOOLTAG              "_bool_tag"
#define STRINGTAG            "_string_tag"
#define HEAP_START           "heap_start"
#define INHERITANCE_TREE	 "inheritance_tree"

// Naming conventions
#define DISPTAB_SUFFIX       "_dispTab"
#define DISPENT_PREFIX       "_dispTab."
#define METHOD_SEP           "."
#define CLASSINIT_SUFFIX     "_init"
#define PROTOBJ_SUFFIX       "_protObj"
#define OBJECTPROTOBJ        "Object_protObj"
#define INTCONST_PREFIX      "int_const"
#define STRCONST_PREFIX      "str_const"
#define BOOLCONST_PREFIX     "bool_const"


#define EMPTYSLOT            0
#define LABEL                ":\n"

#define STRINGNAME ((char *) "String")
#define INTNAME    ((char *) "Int")
#define BOOLNAME   ((char *) "Bool")
#define MAINNAME   ((char *) "Main")

//
// information about object headers
//
#define DEFAULT_OBJFIELDS 3
#define TAG_OFFSET 0
#define SIZE_OFFSET 1
#define DISPTABLE_OFFSET 2

#define STRING_SLOTS      1
#define INT_SLOTS         1
#define BOOL_SLOTS        1

// DIRECTIVES
#define DW            "\t.dw\t"
#define DB            "\t.db\t"

#define BREAK		  "\tBREAK"

// PREPROCESSOR
#define INCLUDE	"#include\t"
#define DEFINE    "#define\t"

//
// register names
//
// Register const ZERO = "$zero";		// Zero register 
// Register const ACC  = "$a0";		// Accumulator 
// Register const A1   = "$a1";		// For arguments to prim functions
// Register const SELF = "$s0";		// Pointer to self (callee saves)
// Register const T1   = "$t1";		// Temporary 1 
// Register const T2   = "$t2";		// Temporary 2 
// Register const T3 	= "$t3";		// Temporary 3
// Register const T5	= "$t5";		// Temporary 5 -- not destroyed by support code functions
// Register const SP   = "$sp";		// Stack pointer 
// Register const FP   = "$fp";		// Frame pointer 
// Register const RA   = "$ra";		// Return address


//
// Opcodes
//
#define CALL "\tcall\t"
#define BCALL "\tbcall\t"
#define JR   "\tjr\t"
#define JP   "\tjp\t"
#define RET	 "\tret\t"

#define PUSH "\tpush\t"
#define POP  "\tpop\t"

#define LD   "\tld\t"

#define EX   "\tex\t"

#define SCF  "\tscf"
#define CCF  "\tccf"
#define BIT  "\tbit\t"

#define CP   "\tcp\t"
#define DJNZ "\tdjnz\t"
#define RLA  "\trla"
#define RLCA "\trlca"
#define RL   "\trl\t"

#define NEG  "\tneg\t"
#define CPL	 "\tcpl\t"
#define ADD  "\tadd\t"
#define ADC  "\tadc\t"
#define SUB  "\tsub\t"
#define SBC  "\tsbc\t"
#define INC  "\tinc\t"
#define DEC  "\tdec\t"

#define SLA  "\tsla\t"
#define SRA  "\tsra\t"
#define SRL  "\tsrl\t"
#define AND	 "\tand\t"
#define OR   "\tor\t"
#define XOR  "\txor\t"


namespace cool {
/* function prototypes */
void emit_include(const std::string& filename, std::ostream& os);

template <typename T, typename U>
void emit_load(const RegisterValue& dst, const ImmediateValue<T,U>& src, std::ostream& os) {
	assert (dst.size() == src.size());
	os << LD << dst << "," << src << std::endl;
}

template <typename T, typename U>
void emit_load(const RegisterPointer& dst, const ImmediateValue<T,U>& src, std::ostream& os) {
	switch (src.size()) {
	case 1:
		os << LD << dst << "," << src << std::endl;
		break;
	case 2:
		os << LD << dst << "," << src.low() << std::endl;
		os << INC << dst.reg() << std::endl;
		os << LD << dst << "," << src.high() << std::endl;
		os << DEC << dst.reg() << std::endl;
		break;
	default:
		std::string msg = std::string("ImmediateValue (src)must be of size 1 or 2, but is of size ")
                        + std::string(src.size());
		std::cerr << msg << std::endl;
		throw msg;
	}
}

void emit_load(const RegisterValue& dst, const LabelValue& src, std::ostream& os);
void emit_load(const RegisterPointer& dst, const LabelValue& src, std::ostream& os);
void emit_load(const RegisterValue& dst, const RegisterValue& src, std::ostream& os);
void emit_load(const MemoryValue& dst, const RegisterValue& src, std::ostream& os);
void emit_load(const RegisterValue& dst, const MemoryValue& src, std::ostream& os);
void emit_neg(const Register& dst, std::ostream& s);
void emit_cpl(const Register& dst, std::ostream& s);
void emit_add(const RegisterValue& dst, const RegisterValue& src, std::ostream& s);
void emit_add(const RegisterValue& dst, const Immediate8& src, std::ostream& s);
void emit_add(const RegisterValue& dst, const MemoryValue& src, std::ostream& s);
void emit_adc(const RegisterValue& dst, const RegisterValue& src, std::ostream& s);
void emit_adc(const RegisterValue& dst, const Immediate8& src, std::ostream& s);
void emit_adc(const RegisterValue& dst, const MemoryValue& src, std::ostream& s);
bool compatible_SUB(const Value& src);;
void emit_sub(const Value& src, std::ostream& s);
void emit_inc(const Register& dst, std::ostream& s);
void emit_dec(const Register& dst, std::ostream& s);
void emit_sla(const Register8& dst, std::ostream& s);
void emit_sra(const Register8& dst, std::ostream& s);
void emit_srl(const Register8& dst, std::ostream& s);
void emit_jr(const AbsoluteAddress& loc, Flag flag, std::ostream& s);
void emit_jr(int label_number, Flag flag, std::ostream& s);
bool compatible_JP(const MemoryLocation& dst);
void emit_jp(const MemoryLocation& loc, Flag flag, std::ostream& s);
void emit_jp(int label, Flag flag, std::ostream& s);
void emit_return(Flag flag, std::ostream& s);
void emit_call(const AbsoluteAddress& addr, Flag flag, std::ostream& s);
 void emit_bcall(const AbsoluteAddress& addr, std::ostream& s);
void emit_copy(std::ostream& s);
void emit_gc_assign(std::ostream& s);
void emit_equality_test(std::ostream& s);
void emit_case_abort(std::ostream& s);
void emit_case_abort2(std::ostream& s);
void emit_dispatch_abort(std::ostream& s);
std::string label_ref(int l);
void emit_label_ref(int l, std::ostream &s);
void emit_label_def(int l, std::ostream &s);
bool compatible_PUSH(const Register& src);
void emit_push(const Register& src, std::ostream& s);
bool compatible_POP(const Register& dst);
void emit_pop(const Register& dst, std::ostream& s);
void emit_cp(const Register8& src, std::ostream& s);
void emit_cp(const RegisterPointer& src, std::ostream& s);
void emit_or(const Register8& src, std::ostream& s);
void emit_or(const RegisterPointer& src, std::ostream& s);
void emit_fetch_int(const RegisterValue& dst, const MemoryValue& src, std::ostream& s);
void emit_store_int(const RegisterValue& src, const MemoryValue& dst, std::ostream& s);
void emit_fetch_bool(const RegisterValue& dst, const MemoryValue& src, std::ostream& s);
void emit_store_bool(const RegisterValue& src, const MemoryValue& dst, std::ostream& s);
 void emit_protobj_ref(Symbol* sym, std::ostream& os);
 void emit_disptable_ref(Symbol* sym, std::ostream& os);
 void emit_method_ref(Symbol* classname, Symbol* methodname, std::ostream& os);
 void emit_init_ref(Symbol* sym, std::ostream& os);
 std::string get_init_ref(Symbol* sym);
 void emit_init(Symbol* classname, std::ostream& os);

 std::ostream& CgenRef(std::ostream& os, const StringEntry* entry);
 std::string CgenRef(const StringEntry* entry);
 std::ostream& CgenRef(std::ostream& os, const Int16Entry* entry);
 std::string CgenRef(const Int16Entry* entry);
 std::ostream& CgenRef(std::ostream& os, bool entry);
 std::string CgenRef(bool entry);
 std::ostream& CgenDef(std::ostream& os, const StringEntry* entry, std::size_t class_tag);
 std::ostream& CgenDef(std::ostream& os, const Int16Entry* entry, std::size_t class_tag);
 std::ostream& CgenDef(std::ostream& os, bool entry, std::size_t class_tag);

/**
 * Generate definitions for the constants in a SymbolTable
 * @param os std::ostream to write generated code to
 * @param table SymbolTable of constants, e.g. gIntTable
 * @param class_tag Class tag for the type of constant being generated
 * @return os
 */
template <class Elem>
std::ostream& CgenDef(std::ostream& os, const SymbolTable<Elem>& table, size_t class_tag) {
  std::vector<Elem*> values;
  for (const auto & value : table) {
    values.push_back(value.second.get());
  }
  // Reverse sort by index to maintain backward compatibility with cool
  std::sort(values.begin(), values.end(), [](const Elem* lhs, const Elem* rhs) {
    return lhs->id() > rhs->id();
  });
  for (const auto & value : values) {
    CgenDef(os, value, class_tag);
  }
  return os;
}
 
}

#endif
