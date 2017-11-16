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

#include <iostream>
#include <algorithm>
#include "cgen.h"


Memmgr cgen_Memmgr = GC_NOGC;      // enable/disable garbage collection
Memmgr_Test cgen_Memmgr_Test = GC_NORMAL;  // normal/test GC
Memmgr_Debug cgen_Memmgr_Debug = GC_QUICK; // check heap frequently

bool cgen_optimize = false;       // optimize switch for code generator
bool disable_reg_alloc=false;     // Don't do register allocation


extern void emit_string_constant(std::ostream& str, const char *s);

static const char *gc_init_names[] =
    { "_NoGC_Init", "_GenGC_Init", "_ScnGC_Init" };
static const char *gc_collect_names[] =
    { "_NoGC_Collect", "_GenGC_Collect", "_ScnGC_Collect" };


// The following temporary name will not conflict with any
// user-defined names.
#define TEMP1 "_1"

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

static void emit_load(const char *dest_reg, int offset, const char *source_reg, std::ostream& s)
{
  s << LW << dest_reg << " " << offset * WORD_SIZE << "(" << source_reg << ")"
    << std::endl;
}

static void emit_store(const char *source_reg, int offset, const char *dest_reg, std::ostream& s)
{
  s << SW << source_reg << " " << offset * WORD_SIZE << "(" << dest_reg << ")"
    << std::endl;
}

static void emit_load_imm(const char *dest_reg, int val, std::ostream& s)
{ s << LI << dest_reg << " " << val << std::endl; }

static void emit_load_address(const char *dest_reg, const char *address, std::ostream& s)
{ s << LA << dest_reg << " " << address << std::endl; }

static void emit_partial_load_address(const char *dest_reg, std::ostream& s)
{ s << LA << dest_reg << " "; }


static void emit_move(const char *dest_reg, const char *source_reg, std::ostream& s)
{
  if (regEq(dest_reg, source_reg)) {
    if (cool::gCgenDebug) {
      std::cerr << "    Omitting move from "
                << source_reg << " to " << dest_reg << std::endl;
      s << "#";
    } else
      return;
  }
  s << MOVE << dest_reg << " " << source_reg << std::endl;
}

static void emit_neg(const char *dest, const char *src1, std::ostream& s)
{ s << NEG << dest << " " << src1 << std::endl; }

static void emit_add(const char *dest, const char *src1, const char* src2, std::ostream& s)
{ s << ADD << dest << " " << src1 << " " << src2 << std::endl; }

static void emit_addu(const char *dest, const char *src1, const char* src2, std::ostream& s)
{ s << ADDU << dest << " " << src1 << " " << src2 << std::endl; }

static void emit_addiu(const char *dest, const char *src1, int imm, std::ostream& s)
{ s << ADDIU << dest << " " << src1 << " " << imm << std::endl; }

static void emit_binop(const char* op, const char *dest, const char *src1, const char* src2, std::ostream& s)
{ s << op << dest << " " << src1 << " " << src2 << std::endl; }

static void emit_div(const char *dest, const char *src1, const char* src2, std::ostream& s)
{ s << DIV << dest << " " << src1 << " " << src2 << std::endl; }

static void emit_mul(const char *dest, const char *src1, const char* src2, std::ostream& s)
{ s << MUL << dest << " " << src1 << " " << src2 << std::endl; }

static void emit_sub(const char *dest, const char *src1, const char* src2, std::ostream& s)
{ s << SUB << dest << " " << src1 << " " << src2 << std::endl; }

static void emit_sll(const char *dest, const char *src1, int num, std::ostream& s)
{ s << SLL << dest << " " << src1 << " " << num << std::endl; }

static void emit_jalr(const char *dest, std::ostream& s)
{ s << JALR << "\t" << dest << std::endl; }

static void emit_return(std::ostream& s)
{ s << RET << std::endl; }

static void emit_copy(std::ostream& s)
{ s << JAL << "Object.copy" << std::endl; }

static void emit_gc_assign(std::ostream& s)
{ s << JAL << "_GenGC_Assign" << std::endl; }

static void emit_equality_test(std::ostream& s)
{ s << JAL << "equality_test" << std::endl; }

static void emit_case_abort(std::ostream& s)
{ s << JAL << "_case_abort" << std::endl; }

static void emit_case_abort2(std::ostream& s)
{ s << JAL << "_case_abort2" << std::endl; }

static void emit_dispatch_abort(std::ostream& s)
{ s << JAL << "_dispatch_abort" << std::endl; }

static void emit_label_ref(int l, std::ostream &s)
{ s << "label" << l; }

static void emit_label_def(int l, std::ostream &s)
{
  emit_label_ref(l,s);
  s << ":" << std::endl;
}

static void emit_beqz(const char *source, int label, std::ostream &s)
{
  s << BEQZ << source << " ";
  emit_label_ref(label,s);
  s << std::endl;
}

static void emit_beq(const char *src1, const char* src2, int label, std::ostream &s)
{
  s << BEQ << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << std::endl;
}

static void emit_bne(const char *src1, const char* src2, int label, std::ostream &s)
{
  s << BNE << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << std::endl;
}

static void emit_bleq(const char *src1, const char* src2, int label, std::ostream &s)
{
  s << BLEQ << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << std::endl;
}

static void emit_blt(const char *src1, const char* src2, int label, std::ostream &s)
{
  s << BLT << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << std::endl;
}

static void emit_blti(const char *src1, int imm, int label, std::ostream &s)
{
  s << BLT << src1 << " " << imm << " ";
  emit_label_ref(label,s);
  s << std::endl;
}

static void emit_bgti(const char *src1, int imm, int label, std::ostream &s)
{
  s << BGT << src1 << " " << imm << " ";
  emit_label_ref(label,s);
  s << std::endl;
}

static void emit_branch(int l, std::ostream& s)
{
  s << BRANCH;
  emit_label_ref(l,s);
  s << std::endl;
}

// Push a register on the stack. The stack grows towards smaller addresses.
//
static void emit_push(const char *reg, std::ostream& str)
{
  emit_store(reg,0,SP,str);
  emit_addiu(SP,SP,-4,str);
}

// Fetch the integer value in an Int object.
//
static void emit_fetch_int(const char *dest, const char *source, std::ostream& s)
{ emit_load(dest, DEFAULT_OBJFIELDS, source, s); }

// Update the integer value in an int object.
//
static void emit_store_int(const char *source, const char *dest, std::ostream& s)
{ emit_store(source, DEFAULT_OBJFIELDS, dest, s); }


static void emit_gc_check(const char *source, std::ostream &s)
{
  if (source != A1) emit_move(A1, source, s);
  s << JAL << "_gc_check" << std::endl;
}



namespace cool {

bool gCgenDebug = false;

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

namespace {

void emit_protobj_ref(Symbol* sym, std::ostream& os) { os << sym << PROTOBJ_SUFFIX; }
void emit_disptable_ref(Symbol* sym, std::ostream& os) { os << sym << DISPTAB_SUFFIX; }
void emit_method_ref(Symbol* classname, Symbol* methodname, std::ostream& os) {
  os << classname << METHOD_SEP << methodname;
}

void emit_init_ref(Symbol* sym, std::ostream& os) { os << sym << CLASSINIT_SUFFIX; }
void emit_init(Symbol* classname, std::ostream& os) {
  os << JAL; emit_init_ref(classname, os); os << std::endl;
}

/**
 * Generate reference to label for String constant
 * @param os std::ostream to write generated code to
 * @param entry String constant
 * @return os
 */
std::ostream& CgenRef(std::ostream& os, const StringEntry* entry) {
  os << STRCONST_PREFIX << entry->id();
  return os;
}

/**
 * Generate reference to label for Int constant
 * @param os std::ostream to write generated code to
 * @param entry Int constant
 * @return os
 */
std::ostream& CgenRef(std::ostream& os, const Int32Entry* entry) {
  os << INTCONST_PREFIX << entry->id();
  return os;
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
  os << WORD << "-1" << std::endl;
  CgenRef(os, entry) << LABEL;
  os << WORD << class_tag << std::endl
     << WORD << (DEFAULT_OBJFIELDS + STRING_SLOTS + (value.size() + 4)/4) << std::endl // size
     << WORD; emit_disptable_ref(String, os); os << std::endl;
  os << WORD; CgenRef(os, length_entry) << std::endl;
  emit_string_constant(os, value.c_str());
  os << ALIGN;
  return os;
}

/**
 * Emit definition of integer constant labeled by index in the gIntTable
 * @param os std::ostream to write generated code to
 * @param entry Int constant
 * @param class_tag Int class tag
 * @return os
 */
std::ostream& CgenDef(std::ostream& os, const Int32Entry* entry, std::size_t class_tag) {
  // Add -1 eye catcher
  os << WORD << "-1" << std::endl;
  CgenRef(os, entry) << LABEL;
  os << WORD << class_tag << std::endl
     << WORD << (DEFAULT_OBJFIELDS + INT_SLOTS) << std::endl
     << WORD; emit_disptable_ref(Int, os); os << std::endl;
  os << WORD << entry->value() << std::endl;
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
  os << WORD << "-1" << std::endl;
  CgenRef(os, entry) << LABEL;
  os << WORD << class_tag << std::endl
     << WORD << (DEFAULT_OBJFIELDS + BOOL_SLOTS) << std::endl
     << WORD; emit_disptable_ref(Bool, os); os << std::endl;
  os << WORD << ((entry) ? 1 : 0) << std::endl;
  return os;
}

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


} // anonymous namespace


CgenKlassTable::CgenKlassTable(Klasses* klasses) {
  InstallClasses(klasses);

  // Add your code to:
  // - Build inheritance graph
  // - Initialize the node tags and other information you need
}



void CgenKlassTable::CgenGlobalData(std::ostream& os) const {
  Symbol* main    = gIdentTable.emplace(MAINNAME);
  Symbol* string  = gIdentTable.emplace(STRINGNAME);
  Symbol* integer = gIdentTable.emplace(INTNAME);
  Symbol* boolc   = gIdentTable.emplace(BOOLNAME);

  os << "\t.data\n" << ALIGN;

  // The following global names must be defined first.
  os << GLOBAL << CLASSNAMETAB << std::endl;
  os << GLOBAL; emit_protobj_ref(main, os);    os << std::endl;
  os << GLOBAL; emit_protobj_ref(integer, os); os << std::endl;
  os << GLOBAL; emit_protobj_ref(string, os);  os << std::endl;
  os << GLOBAL << BOOLCONST_PREFIX << 0 << std::endl;
  os << GLOBAL << BOOLCONST_PREFIX << 1 << std::endl;
  os << GLOBAL << INTTAG << std::endl;
  os << GLOBAL << BOOLTAG << std::endl;
  os << GLOBAL << STRINGTAG << std::endl;

  // We also need to know the tag of the Int, String, and Bool classes
  // during code generation.
  os << INTTAG << LABEL << WORD << TagFind(integer) << std::endl;
  os << BOOLTAG << LABEL << WORD << TagFind(boolc) << std::endl;
  os << STRINGTAG << LABEL << WORD <<  TagFind(string) << std::endl;
}

void CgenKlassTable::CgenSelectGC(std::ostream& os) const {
  os << GLOBAL << "_MemMgr_INITIALIZER" << std::endl;
  os << "_MemMgr_INITIALIZER:" << std::endl;
  os << WORD << gc_init_names[cgen_Memmgr] << std::endl;
  os << GLOBAL << "_MemMgr_COLLECTOR" << std::endl;
  os << "_MemMgr_COLLECTOR:" << std::endl;
  os << WORD << gc_collect_names[cgen_Memmgr] << std::endl;
  os << GLOBAL << "_MemMgr_TEST" << std::endl;
  os << "_MemMgr_TEST:" << std::endl;
  os << WORD << (cgen_Memmgr_Test == GC_TEST) << std::endl;
}

void CgenKlassTable::CgenConstants(std::ostream& os) const {
  // Make sure "default" values are in their respective tables
  gStringTable.emplace("");
  gIntTable.emplace(0);

  std::size_t string_tag = TagFind(String), int_tag = TagFind(Int), bool_tag = TagFind(Bool);
  CgenDef(os, gStringTable, string_tag);
  CgenDef(os, gIntTable, int_tag);
  CgenDef(os, false, bool_tag);
  CgenDef(os, true, bool_tag);
}


void CgenKlassTable::CgenGlobalText(std::ostream& os) const {
  os << GLOBAL << HEAP_START << std::endl
      << HEAP_START << LABEL
      << WORD << 0 << std::endl
      << "\t.text" << std::endl
      << GLOBAL;
  emit_init_ref(Main, os);
  os << std::endl << GLOBAL;
  emit_init_ref(Int, os);
  os << std::endl << GLOBAL;
  emit_init_ref(String, os);
  os << std::endl << GLOBAL;
  emit_init_ref(Bool, os);
  os << std::endl << GLOBAL;
  emit_method_ref(Main, main_meth, os);
  os << std::endl;
}



void CgenKlassTable::CodeGen(std::ostream& os) const {
  CgenGlobalData(os);
  CgenSelectGC(os);
  CgenConstants(os);

  // Add your code to emit:
  // - Prototype objects
  // - class_nameTab and class_objTab
  // - Dispatch tables for each class


  CgenGlobalText(os);

  // Add your code to emit:
  // - Object initializers for each class
  // - Class methods

}


void Cgen(Program* program, std::ostream& os) {
  InitCoolSymbols();

  CgenKlassTable klass_table(program->klasses());
  klass_table.CodeGen(os);
}



}
