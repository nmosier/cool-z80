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

#include <map>


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

static void emit_jr(const char *dest, std::ostream& s)
{ s << JR << dest << std::endl; }

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

static void emit_bltz(const char *src1, int label, std::ostream &s)
{
  s << BLTZ << src1 << " ";
  emit_label_ref(label, s);
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

// Pop word from stack into register
static void emit_pop(const char*dest_reg, std::ostream& str) {
  emit_addiu(SP,SP,4,str);
  emit_load(dest_reg, 0, SP, str);
}

// Fetch the integer value in an Int object.
//
static void emit_fetch_int(const char *dest, const char *source, std::ostream& s)
{ emit_load(dest, DEFAULT_OBJFIELDS, source, s); }

// Update the integer value in an int object.
//
static void emit_store_int(const char *source, const char *dest, std::ostream& s)
{ emit_store(source, DEFAULT_OBJFIELDS, dest, s); }

// Fetch the bool value in a Bool object.
static void emit_fetch_bool(const char* dest, const char* src, std::ostream& s)
{ emit_load(dest, DEFAULT_OBJFIELDS, src, s); }

// Update the bool value in a Bool object.
static void emit_store_bool(const char* src, const char* dest, std::ostream& s)
{ emit_store(src, DEFAULT_OBJFIELDS, dest, s); }


static void emit_gc_check(const char *source, std::ostream &s)
{
  if (source != A1) emit_move(A1, source, s);
  s << JAL << "_gc_check" << std::endl;
}

namespace cool {

bool gCgenDebug = false;
int label_counter = 0;

DispatchTables gCgenDispatchTables;
CgenKlassTable* gCgenKlassTable;

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




/* MemoryLocation implementation */
void IndirectLocation::emit_store_address_to_loc(const char* dest_reg, std::ostream& s) {
  emit_addiu(dest_reg, base_reg(), WORD_SIZE*offset(), s);
}
void IndirectLocation::emit_load_from_loc(const char* dest_reg, std::ostream& s) {
  emit_load(dest_reg, offset(), base_reg(), s);
}
void IndirectLocation::emit_store_to_loc(const char* src_reg, std::ostream& s) {
  emit_store(src_reg, offset(), base_reg(), s);
}

void AbsoluteLocation::emit_store_address_to_loc(const char* dest_reg, std::ostream& s) {
  emit_partial_load_address(dest_reg, s);
  s << label_ << "+" << WORD_SIZE*offset() << std::endl;
}
void AbsoluteLocation::emit_load_from_loc(const char* dest_reg, std::ostream& s) {
  s << LW << dest_reg << " " << label() << "+" << offset() << std::endl;
}
void AbsoluteLocation::emit_store_to_loc(const char* src_reg, std::ostream& s) {
  s << SW << src_reg << " " << label() << "+" << WORD_SIZE*offset() << std::endl;
}



// CreateAttrVarEnv
//  -adds attributes to the variable environment (mapping from symbols to mem locations)
void CgenNode::CreateAttrVarEnv(int next_offset) {
  if (parent() != nullptr) {
  	attrVarEnv_ = parent_->attrVarEnv_;
  	objectSize_ = parent_->objectSize_;
  }
  
  for (Features::const_iterator feature = klass()->features_begin(); feature != klass()->features_end(); ++feature) {
  	if ((*feature)->attr()) {
  	  Attr* attr = (Attr*) (*feature);
  	  MemoryLocation* offset = new IndirectLocation(next_offset, SELF);
  	  attrVarEnv_.Push(attr->name(), offset);  	  
  	  ++next_offset;
  	  ++objectSize_;	// update object size
  	}
  }
  
  for (CgenNode* child : children_)
  	{ child->CreateAttrVarEnv(next_offset); }
}

// CreateDispatchTables
//  -creates a table of dispatch tables (one per class)
//   with each table mapping a method name to a memory location
void CgenNode::CreateDispatchTables(DispatchTables& tables, int next_offset) {
  if (parent() != nullptr)
     { tables[klass()->name()] = tables[parent()->name()]; }
  
  for (Features::const_iterator feature = klass()->features_begin(); feature != klass()->features_end(); ++feature) {
  	if ((*feature)->method()) {
  	  Method* method = (Method*) (*feature);
  	  if (tables[klass()->name()].find(method->name()) == tables[klass()->name()].end()) {
  	    std::string dispTab_label = klass()->name()->value();
  	    dispTab_label.append(DISPTAB_SUFFIX);
  	    
  	    MemoryLocation* offset = new AbsoluteLocation(next_offset, dispTab_label);
  	    tables[klass()->name()].emplace(method->name(), offset);
  	    ++next_offset;
  	  }
  	}
  }
  
  for (CgenNode* child : children_)
    { child->CreateDispatchTables(tables, next_offset); }
}




// CgenKlassTable initializer
//  -builds inheritance graph
//  -assigns class tags
//  -initializes variable environments for each class
//  -creates dispatch tables for each class
CgenKlassTable::CgenKlassTable(Klasses* klasses) {
  InstallClasses(klasses);

  // build inheritance graph (connect nodes) and assign tags
  unsigned int tag = 0;
  for (CgenNode* isolated_node : nodes_) {
    std::clog << isolated_node->klass()->name() << ":" << tag << std::endl;
  	isolated_node->tag_ = tag++;
    CgenNode* parent_node = ClassFind(isolated_node->parent_name());
    parent_node->children_.push_back(isolated_node);
    isolated_node->parent_ = parent_node;
  }
  
  // generate class variable environments, starting recursively from root (Object)
  root()->CreateAttrVarEnv(CgenObjectLayout_AttributeOffset);
  // recursively generate dispatch tables, starting from empty table
  root()->CreateDispatchTables(gCgenDispatchTables, 0);
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


void CgenNode::EmitDispatchTable(std::ostream& os, DispatchTables& tables, MethodInheritanceTable inheritance_t) {
  // add overridden methods to method inheritance table
  for (auto features_it = klass()->features_begin(); features_it != klass()->features_end(); features_it++) {
    if ((*features_it)->method()) {
      Method* method = (Method*) (*features_it);
      Symbol* name = method->name();
      inheritance_t[name] = klass()->name();
    }
  }

  DispatchTable& table = tables[klass()->name()];
  std::vector<Symbol*> ordered_table(table.size());
  for (std::pair<Symbol*,MemoryLocation*> p : table)
    {  ordered_table[p.second->offset()] = p.first; }

  os << klass()->name() << DISPTAB_SUFFIX << LABEL;
  for (Symbol* method_name : ordered_table)
    { os << WORD << inheritance_t[method_name] << METHOD_SEP << method_name << std::endl; }

  for (CgenNode* child : children_)
    { child->EmitDispatchTable(os, tables, inheritance_t); }
}

// CgenDispatchTables: emits all dispatch tables
void CgenKlassTable::CgenDispatchTables(std::ostream& os) const {
  CgenNode::MethodInheritanceTable inheritance_t;
  root()->EmitDispatchTable(os, gCgenDispatchTables, inheritance_t);
}

void CgenNode::EmitPrototypeObject(std::ostream& os) {
  // handle Int, String, Bool separately
  os << WORD << "-1" << std::endl;	// GC tag
  os << klass()->name() << PROTOBJ_SUFFIX << LABEL; // label
  os << WORD << tag_ << std::endl; // tag
  if (klass()->name() == String) {
    os << WORD << 5 << std::endl;
    os << WORD << klass()->name() << DISPTAB_SUFFIX << std::endl;
    os << WORD;
    CgenRef(os, gIntTable.lookup(0)) << std::endl;
    os << WORD << 0 << std::endl;
  } else if (klass()->name() == Int || klass()->name() == Bool) {
    // attributes end up being the same for Int & Bool
    os << WORD << 4 << std::endl;
    os << WORD << klass()->name() << DISPTAB_SUFFIX << std::endl;
    os << WORD << 0 << std::endl;
  } else {
    os << WORD << objectSize_ << std::endl;
    os << WORD << klass()->name() << DISPTAB_SUFFIX << std::endl;
    for (int i = 0; i < attrVarEnv_.vars_.size(); ++i)
    	{ os << WORD << 0 << std::endl; }
  }
  
  for (CgenNode* child : children_)
    { child->EmitPrototypeObject(os); }
}

// CgenPrototypeObjects: emit prototype objects for all classes
void CgenKlassTable::CgenPrototypeObjects(std::ostream& os) const {
  root()->EmitPrototypeObject(os);
}

// CgenClassNameTab: emit class name table
void CgenKlassTable::CgenClassNameTab(std::ostream& os) const {
  std::map<std::size_t,Symbol*> ordered_class_names;
  for (CgenNode* node : nodes_)
    { ordered_class_names[node->tag_] = node->klass()->name(); }
    
  for (std::pair<std::size_t,Symbol*> p : ordered_class_names) {
    bool had_length_entry = gIntTable.has(p.second->value().size());
    gStringTable.emplace(p.second->value());
    Symbol* class_name = gStringTable.lookup(p.second->value());
    CgenDef(os, class_name, TagFind(String));
    if (!had_length_entry) {
      // int constant for str len needs to be generated
      Int32Entry* length_entry = gIntTable.lookup(class_name->value().size());
      CgenDef(os, length_entry, TagFind(Int));
    }
  }
  
  os << CLASSNAMETAB << LABEL;
  for (std::pair<std::size_t,Symbol*> p : ordered_class_names) {
    os << WORD;
    Symbol* class_name = gStringTable.lookup(p.second->value());
    CgenRef(os, class_name);
    os << std::endl;
  }
}

// CgenClassObjTab: emit class object table
void CgenKlassTable::CgenClassObjTab(std::ostream& os) const {
  std::map<int,Symbol*> ordered_class_names;
  for (CgenNode* node : nodes_) {
    ordered_class_names[node->tag_] = node->klass()->name();
  }
  
  os << CLASSOBJTAB << LABEL;
  for (std::pair<int,Symbol*> p : ordered_class_names) {
    os << WORD << p.second << PROTOBJ_SUFFIX << std::endl;
    os << WORD << p.second << CLASSINIT_SUFFIX << std::endl;
  }
}


// EmitInitializer: emit initializer for class
void CgenNode::EmitInitializer(std::ostream& os) {
  attrVarEnv_.klass_ = klass(); // set current class
  attrVarEnv_.ResetTemporaryCount(); // so temporaries will be assigned to proper locs
  os << klass()->name() << CLASSINIT_SUFFIX << LABEL;

  if (klass()->name() == Object) {
  	// do nothing
  } else {
    Symbol* parent_name = parent()->klass()->name();
    
    // set up activation record
    emit_addiu(SP, SP, -WORD_SIZE*CgenARLayout_BaseSize, os);
    emit_store(FP, CgenARLayout_FPOffset+1, SP, os);
    emit_store(SELF, CgenARLayout_SelfPOffset+1, SP, os);
    emit_addiu(FP, SP, WORD_SIZE*1, os); // set FP to caller's saved RA
    
    // find maximum number of temporaries needed over all attributes
    int max_temps = 0;
    for (Features::const_iterator feature = klass()->features_begin(); feature != klass()->features_end(); ++feature) {
      if ((*feature)->attr()) {
        Attr* attr = (Attr*) (*feature);
        max_temps = std::max(max_temps, attr->init()->CalcTemps());
      }
    }
    emit_addiu(SP, SP, -max_temps*WORD_SIZE, os); // reserve space for temps
    
    // call parent initializer
    emit_push(RA, os);
    os << JAL;
	emit_init_ref(parent_name, os);
	os << std::endl;
	emit_pop(RA, os);
	
	// set self for attribute initializers
	emit_move(SELF, ACC, os);
	
	// initialize attributes
	for (Features::const_iterator feature = klass()->features_begin(); feature != klass()->features_end(); ++feature) {
	  if ((*feature)->attr()) {
	    Attr* attr = (Attr*) (*feature);
	    Expression* init = attr->init();
	    attrVarEnv_.init_type_ = attr->decl_type();    
	    init->CodeGen(attrVarEnv_, os);	// result in ACC
	    MemoryLocation* attr_offset = attrVarEnv_.Lookup(attr->name());	    
	    attr_offset->emit_store_to_loc(ACC, os);
	  }
	}
	
	emit_move(ACC, SELF, os); // current object expected in ACC
	
	// cleanup
	emit_load(SELF, CgenARLayout_SelfPOffset, FP, os);
	emit_load(FP, CgenARLayout_FPOffset, FP, os);
	emit_addiu(SP, SP, WORD_SIZE*(CgenARLayout_BaseSize+max_temps), os);
	
	attrVarEnv_.init_type_ = nullptr;
  }
  emit_return(os);
  
  for (CgenNode* child : children_) {
    child->EmitInitializer(os);
  }
}

// CgenClassInits: emits initializers for all classes
void CgenKlassTable::CgenClassInits(std::ostream& os) const {
  root()->EmitInitializer(os);
}


void CgenNode::EmitMethods(std::ostream& os) {
  if (!basic()) {
    for (Features::const_iterator feat_it = klass()->features_begin(); feat_it != klass()->features_end(); ++feat_it) {
      if ((*feat_it)->method()) {
        Method* method = (Method*) (*feat_it);
        os << klass()->name() << METHOD_SEP << method->name() << LABEL;
        attrVarEnv_.klass_ = klass();
        method->CodeGen(attrVarEnv_, os);
      }
    }
  }
  
  for (CgenNode* child : children_) {
    child->EmitMethods(os);
  }
}

// CgenClassMethods: emit methods for all classes
void CgenKlassTable::CgenClassMethods(std::ostream& os) const {
  root()->EmitMethods(os);
}

void CgenKlassTable::CodeGen(std::ostream& os) const {
  CgenGlobalData(os);
  CgenSelectGC(os);
  CgenConstants(os);
  
  CgenPrototypeObjects(os);
  CgenDispatchTables(os);
  CgenClassObjTab(os);
  CgenClassNameTab(os);

  CgenGlobalText(os);
  
  CgenClassInits(os);
  CgenClassMethods(os);
}



/* CODE GENERATION FOR AST NODES */
void Method::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  // need to add formals to variable environment
  // don't need to worry about binding self in codegen
  varEnv.ResetTemporaryCount();
  int temp_count = body_->CalcTemps();
  
  // perform callee setup
  emit_addiu(SP, SP, -WORD_SIZE*CgenARLayout_BaseSize, os);
  
  emit_store(FP, CgenARLayout_FPOffset+1, SP, os);
  emit_store(SELF, CgenARLayout_SelfPOffset+1, SP, os);
  emit_addiu(FP, SP, WORD_SIZE*1, os); // set FP to caller's saved RA
  emit_addiu(SP, SP, -WORD_SIZE*temp_count, os);
  
  emit_move(SELF, ACC, os); // bind self
    
  int formals_counter = formals()->size() - 1 + CgenARLayout_BaseSize; // account for storage of caller's state
  for (Formals::const_iterator formals_it = formals_begin(); formals_it != formals_end(); ++formals_it) {
    Formal* formal = *formals_it;
    // need to assign location relative to FP
    MemoryLocation* formal_loc = new IndirectLocation(formals_counter, FP);
    varEnv.Push(formal->name(), formal_loc);
    --formals_counter;
  }
  
  body_->CodeGen(varEnv, os); // generate method body
  int temporaryOffset = varEnv.GetTemporaryMaxCount() * WORD_SIZE;
    
  emit_load(SELF, CgenARLayout_SelfPOffset, FP, os);
  emit_load(FP, CgenARLayout_FPOffset, FP, os);
  
  // pop entire AR off stack, including arguments from caller
  emit_addiu(SP, SP, WORD_SIZE*CgenARLayout_BaseSize + temporaryOffset + WORD_SIZE*formals_->size(), os);
  emit_return(os);
  
  // delete MemoryLocation ptrs
  for (Formals::const_iterator formals_it = formals_begin(); formals_it != formals_end(); ++formals_it) {
    Formal* formal = *formals_it;
    MemoryLocation* formal_loc = varEnv.Lookup(formal->name());
    varEnv.Pop(formal->name());
    delete formal_loc;
  }
}

void BoolLiteral::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  emit_partial_load_address(ACC, os);
  CgenRef(os, value()) << std::endl;
}

void IntLiteral::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  emit_partial_load_address(ACC, os);
  CgenRef(os, value_) << std::endl;
}

void StringLiteral::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  emit_partial_load_address(ACC, os);
  CgenRef(os, value_) << std::endl;
}

void NoExpr::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  // need to know type of relevant variable
  Symbol* init_t = varEnv.init_type_;
  if (init_t == Int) {
    emit_partial_load_address(ACC, os);
    CgenRef(os, gIntTable.lookup(0)); os << std::endl;
  } else if (init_t == Bool) {
    emit_partial_load_address(ACC, os);
    CgenRef(os, false); os << std::endl;
  } else if (init_t == String) {
    emit_partial_load_address(ACC, os);
    CgenRef(os, gStringTable.lookup(std::string(""))); os << std::endl;
  } else {
    emit_move(ACC, ZERO, os);	// Void pointer is default initialization
  }
}

void Ref::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  if (name_ == self) {
    emit_move(ACC, SELF, os);
  } else {
    MemoryLocation* offset = varEnv.Lookup(name_);
    offset->emit_load_from_loc(ACC, os);
  }
}

void BinaryOperator::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  // is the resulting object a Bool? (otherwise an Int)
  bool bool_result = (kind_ == BO_LT || kind_ == BO_EQ || kind_ == BO_LE);
  
  emit_partial_load_address(ACC, os);
  os << (bool_result? BOOLNAME : INTNAME) << PROTOBJ_SUFFIX << std::endl;
  emit_move(T5, RA, os);
  emit_copy(os);
  emit_move(RA, T5, os);
  emit_push(ACC, os);	// push new object (of Bool or Int type) onto stack
  
  lhs_->CodeGen(varEnv, os); // evaluate left first
  emit_push(ACC, os);
  rhs_->CodeGen(varEnv, os); // evaluate right
  emit_pop(T1, os);
  emit_move(T2, ACC, os);
  
  if (kind_ != BO_EQ && lhs_->type() == Int && rhs_->type() == Int) {
    // fetch int32 values into T1 & T2
    emit_fetch_int(T1, T1, os);
    emit_fetch_int(T2, T2, os);
    
    switch (kind_) {
    case BO_Add:
      emit_addu(ACC, T1, T2, os);
      break;
    case BO_Sub:
      emit_sub(ACC, T1, T2, os);
      break;
    case BO_Mul:
      emit_mul(ACC, T1, T2, os);
      break;
    case BO_Div:
      emit_div(ACC, T1, T2, os);
      break;
    case BO_LT:
      emit_load_imm(ACC, 1, os);
      emit_blt(T1, T2, label_counter, os);
      emit_move(ACC, ZERO, os);
      emit_label_def(label_counter++, os); 
      break;
    case BO_LE:
      emit_load_imm(ACC, 1, os);
      emit_bleq(T1, T2, label_counter, os); // TBI
      emit_move(ACC, ZERO, os);
      emit_label_def(label_counter++, os);
      break;
    default:
      std::clog << "BinaryOperator::CodeGen - default branch in switch should never be reached." << std::endl;
    }
    emit_pop(T1, os); // pop new object into ACC
    if (bool_result) {
      emit_store_bool(ACC, T1, os);
    } else {
      emit_store_int(ACC, T1, os); // store result in T1 to Int obj in ACC
    }
    emit_move(ACC, T1, os);
  } else {
    // equality operator
    const int equal_end = label_counter++;
    
    emit_move(T5, RA, os);
    emit_move(A1, ZERO, os);	// FALSE
    emit_load_imm(ACC, 1, os); // TRUE
    emit_beq(T1, T2, equal_end, os); // pointer equality => object equality (for basic & non-basic types)
    emit_equality_test(os); // returns FALSE if objects of non-basic, mismatching type or otherwise unequal
    emit_move(RA, T5, os);
    
    emit_label_def(equal_end, os);
    emit_pop(T1, os); // Bool object to store to
    emit_store_bool(ACC, T1, os); // store result into T1
    emit_move(ACC, T1, os); // final result in ACC
  }
}

void UnaryOperator::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  bool bool_result = (kind_ != UO_Neg);
  
  emit_partial_load_address(ACC, os);
  os << (bool_result? BOOLNAME : INTNAME) << PROTOBJ_SUFFIX << std::endl;
  emit_move(T5, RA, os);
  emit_copy(os);
  emit_move(RA, T5, os);
  emit_push(ACC, os);	// push new object (of Bool or Int type) onto stack
  
  // evaluate input expression
  input_->CodeGen(varEnv, os);
  switch (kind_) {
  case UO_Neg:
    emit_fetch_int(T1, ACC, os);
    emit_neg(T1, T1, os);
    break;
  case UO_Not:
    emit_fetch_bool(T1, ACC, os);
    os << XORI << T1 << " " << T1 << " " << 1 << std::endl;    // use XOR to flip bool val
    break;
  case UO_IsVoid:
    emit_load_imm(T1, 1, os);	// T1 <- true
    os << MOVN << T1 << " " << ZERO << " " << ACC << std::endl;	// if ACC points to object, then T1 <- false 
    break;
  }
  emit_pop(ACC, os);
  if (bool_result) {
    emit_store_bool(T1, ACC, os);
  } else {
    emit_store_int(T1, ACC, os);
  }
}

void Knew::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  if (name_ == SELF_TYPE) {
    emit_load(ACC, TAG_OFFSET, SELF, os);
    emit_sll(ACC, ACC, LOG_WORD_SIZE+1, os); // offset of protObj of dynamic type
    emit_partial_load_address(T5, os);
    os << CLASSOBJTAB << std::endl;
    emit_addu(T5, T5, ACC, os);
    emit_load(ACC, 0, T5, os);
    
    emit_push(RA, os);
    emit_copy(os); // ACC contains pointer to uninitialized copy
    emit_addiu(T5, T5, 4, os);  // now points to init method of dynamic type
    emit_load(T1, 0, T5, os); // $t1 now contains address of init method
    emit_jalr(T1, os);
    emit_pop(RA, os);
  } else {
    emit_partial_load_address(ACC, os);
    os << name_ << PROTOBJ_SUFFIX << std::endl;
    emit_push(RA, os);
    emit_copy(os);
    emit_init(name_, os);  
    emit_pop(RA, os);  
  }
}

void KaseBranch::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  // need to store location
  MemoryLocation* case_branch_var = new IndirectLocation(-varEnv.IncTemporaryCount(), FP);
  varEnv.Push(name_, case_branch_var);
  
  // expect address to be in register T1
  case_branch_var->emit_store_to_loc(T1, os); // store to assigned temporary slot
  int class_tag = gCgenKlassTable->TagFind(decl_type_);
  emit_load_imm(T2, class_tag, os);
  emit_store(T2, TAG_OFFSET, T1, os); // change type of object to match case branch
  
  body_->CodeGen(varEnv, os);
  
  varEnv.Pop(name_);
  varEnv.DecTemporaryCount();
  delete case_branch_var;
}

void Kase::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  const int case_loop = label_counter;
  const int case_loop2 = label_counter+1;
  const int case_abort = label_counter+2;
  const int case_abort2 = label_counter+3;
  const int case_table = label_counter+4;
  const int case_esac = label_counter+5;
  label_counter += 6;

  input_->CodeGen(varEnv, os);  // evaluate case expr
  emit_beqz(ACC, case_abort2, os); // abort if void
  emit_load(T1, TAG_OFFSET, ACC, os); // get type of input
  
  // generate sets of class tags that inherit from declared type of each case branch
  std::map<CgenNode::ClassTag, CgenNode::ClassTagSet> class_tag_sets;
  for (KaseBranch* kase_branch : *cases_) {
    CgenNode* node = gCgenKlassTable->ClassFind(kase_branch->decl_type());
    CgenNode::ClassTag tag = gCgenKlassTable->TagFind(kase_branch->decl_type());
    class_tag_sets[tag] = CgenNode::ClassTagSet(); // add to map
    node->GetSubtreeClassTags(class_tag_sets[tag]);
  }
  
  // T1 contains case-expr CLASS id
  // T2 contains tag to set for new obj copy in case branch
  // T3 contains current "candidate" tag
  // T5 contains label for appropriate case branch
  
  // init
  emit_push(ACC, os); // preserve ptr to case expr
  emit_partial_load_address(ACC, os);
  emit_label_ref(case_table, os); os << std::endl;
  // main loop
  emit_label_def(case_loop, os);
  emit_load(T2, 0, ACC, os); // class tag for branch
  emit_bltz(T2, case_abort, os);
  emit_load(T5, 1, ACC, os); // label for branch
  emit_addiu(ACC, ACC, WORD_SIZE*2, os);
  // inner loop
  emit_label_def(case_loop2, os);
  emit_load(T3, 0, ACC, os); // get "candidate" tag
  emit_addiu(ACC, ACC, WORD_SIZE*1, os);
  emit_bltz(T3, case_loop, os); // end of section
  emit_bne(T1, T3, case_loop2, os); // continue in section if tag doesn't match
  
  emit_pop(T1, os); // pop pointer to obj (expected in T1 by case branch)
  emit_jr(T5, os);	  // execute branch
  
  // case_abort: no match found
  emit_label_def(case_abort, os);
  emit_pop(ACC, os);
  emit_jalr("_case_abort", os);
  
  // case_abort_2: case on void expr
  emit_label_def(case_abort2, os);
  emit_load_imm(T1, this->loc(), os);
  emit_partial_load_address(ACC, os);
  CgenRef(os, gStringTable.lookup(varEnv.klass_->filename()->value()));
  os << std::endl;
  emit_jalr("_case_abort2", os);
  
  std::map<CgenNode::ClassTag,int> tags_to_labels;
  // now emit code for case branches
  for (KaseBranch* branch : *cases_) {
    CgenNode::ClassTag branch_tag = gCgenKlassTable->TagFind(branch->decl_type());
    int branch_label = label_counter++;
    tags_to_labels[branch_tag] = branch_label;
    emit_label_def(branch_label, os);
    branch->CodeGen(varEnv, os);
    emit_branch(case_esac, os);
  }
  
  // iteratively find shortest list (guaranteed to be least or have no children among remaining types )
  // output list of "qualifying" class tags
  // (use O(n^2) algorithm for now -- maybe improve later?
  emit_label_def(case_table, os); // beginning of table
  while (!class_tag_sets.empty()) {
    int min_size = INT_MAX; CgenNode::ClassTag min_branch;
    for (std::map<CgenNode::ClassTag,CgenNode::ClassTagSet>::iterator set_it = class_tag_sets.begin(); set_it != class_tag_sets.end(); ++set_it) {
      if (set_it->second.size() < min_size) {
        min_branch = set_it->first;
        min_size = class_tag_sets[min_branch].size();
      }
    }
    
    // emit header for each case branch
    // - class tag of branch object
    // - branch label (to jump to)
    os << WORD << min_branch << std::endl;
    os << WORD; emit_label_ref(tags_to_labels[min_branch], os); os << std::endl;
    
    // emit list of matching class tags
    CgenNode::ClassTagSet tag_set = class_tag_sets[min_branch];
    for (CgenNode::ClassTagSet::iterator tag_it = tag_set.begin(); tag_it != tag_set.end(); ++tag_it) {
      os << WORD << *tag_it << std::endl;
    }
    // mark end of qualifying list with -1
    os << WORD << -1 << std::endl;
    class_tag_sets.erase(min_branch);
  }
  // emit final -1 to indicate runtime error, i.e. no branches matched
  os << WORD << -1 << std::endl;
  
  emit_label_def(case_esac, os);	// exit point of case statement
}

void Let::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  // SELF_TYPE not allowed, so don't need to consider
  MemoryLocation* let_var = new IndirectLocation(-varEnv.IncTemporaryCount(), FP);
  varEnv.init_type_ = decl_type_;
  
  init_->CodeGen(varEnv, os);
  varEnv.Push(name_, let_var);
  let_var->emit_store_to_loc(ACC, os);
  
  body_->CodeGen(varEnv, os);
  
  varEnv.Pop(name_);
  varEnv.init_type_ = nullptr;
  varEnv.DecTemporaryCount();
  delete let_var;	// cleanup
}

void Block::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  for (Expression* expr : *body_) {
    expr->CodeGen(varEnv, os);
  }
}

void Loop::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  int label_loop_pred = label_counter++;
  int label_loop_end = label_counter++;
  
  emit_label_def(label_loop_pred, os);
  pred_->CodeGen(varEnv, os);
  emit_fetch_bool(ACC, ACC, os);
  emit_beqz(ACC, label_loop_end, os);
  body_->CodeGen(varEnv, os);
  emit_branch(label_loop_pred, os);
  
  emit_label_def(label_loop_end, os);	// loop done
  emit_move(ACC, ZERO, os); // evaluates to VOID
}

void Cond::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  int label_else = label_counter++;
  int label_fi = label_counter++;
  
  // evaluate predicate
  pred_->CodeGen(varEnv, os); // if
  emit_fetch_bool(ACC, ACC, os); // get bool value
  emit_beqz(ACC, label_else, os); // branch to 'else' if false
  
  then_branch_->CodeGen(varEnv, os); // then
  emit_branch(label_fi, os);
  
  emit_label_def(label_else, os); // else
  else_branch_->CodeGen(varEnv, os);
  
  emit_label_def(label_fi, os); // fi
}

void StaticDispatch::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  const int dispatch_end = label_counter;
  const int dispatch_abort = label_counter+1;
  label_counter += 2;
  
  // 1. evaluate actuals
  // 2. evaluate receiver
  
  emit_push(RA, os);
  for (Expression* expr : *actuals_) {
    expr->CodeGen(varEnv, os);
    emit_push(ACC, os);
  }

  receiver_->CodeGen(varEnv, os);
  emit_beqz(ACC, dispatch_abort, os); // abort if void
  
  // call static method on given class
  emit_partial_load_address(T1, os);
  os << dispatch_type_ << METHOD_SEP << name_ << std::endl;
  emit_partial_load_address(RA, os); emit_label_ref(dispatch_end, os); os << std::endl;
  emit_jr(T1, os);
  
  // dispatch_abort
  emit_label_def(dispatch_abort, os);
  emit_load_imm(T1, this->loc(), os);
  emit_partial_load_address(ACC, os);
  CgenRef(os, gStringTable.lookup(varEnv.klass_->filename()->value()));
  os << std::endl;
  emit_jalr("_dispatch_abort", os);
  
  // dispatch end
  emit_label_def(dispatch_end, os);
  emit_pop(RA, os);
}

void Dispatch::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  const int dispatch_end = label_counter;
  const int dispatch_abort = label_counter+1;
  label_counter += 2;

  emit_push(RA, os);
  for (Expression* expr : *actuals_) {
    expr->CodeGen(varEnv, os);
    emit_push(ACC, os);
  }
  
  receiver_->CodeGen(varEnv, os);
  emit_beqz(ACC, dispatch_abort, os); // check if void
  
  emit_load(T1, DISPTABLE_OFFSET, ACC, os);  // get pointer to dispatch table of receiver
  
  MemoryLocation* method_offset;
  if (receiver_->type() == SELF_TYPE) {
    method_offset = gCgenDispatchTables[varEnv.klass_->name()][name_];
  } else {
    method_offset = gCgenDispatchTables[receiver_->type()][name_];
  }
  emit_addiu(T1, T1, WORD_SIZE*method_offset->offset(), os);
  emit_load(T1, 0, T1, os); // retrieve method address
  emit_partial_load_address(RA, os); emit_label_ref(dispatch_end, os); os << std::endl;
  emit_jr(T1, os);
  
  // dispatch_abort
  emit_label_def(dispatch_abort, os);
  emit_load_imm(T1, this->loc(), os);
  emit_partial_load_address(ACC, os);
  CgenRef(os, gStringTable.lookup(varEnv.klass_->filename()->value()));
  os << std::endl;
  emit_jalr("_dispatch_abort", os);
  
  // dispatch end
  emit_label_def(dispatch_end, os);
  emit_pop(RA, os);
}

void Assign::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  // lookup memory location
  MemoryLocation* loc = varEnv.Lookup(name_);
  value_->CodeGen(varEnv, os);
  loc->emit_store_to_loc(ACC, os);
}


void Cgen(Program* program, std::ostream& os) {
  InitCoolSymbols();

  CgenKlassTable klass_table(program->klasses());
  gCgenKlassTable = &klass_table;
  klass_table.CodeGen(os);
}



}
