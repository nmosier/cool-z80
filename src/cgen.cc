/*
Copyright (c) 1995,1996 The Regents of the University of California.
All rights reserved.

Permission to use, copy, modify, and distribute this software for any
purpose, without fee, and without written agreement is hereby granted,
provided that the above copyright notice and the following two
paragraphs appear in all copies of this software.

IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY
FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

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
#include <fstream>
#include <exception>
#include <algorithm>
#include <assert.h>
#include <map>
#include <cmath>
#include <deque>
#include <set>
#include "emit.h"
#include "cgen.h"
#include "cgen_routines.h"

using namespace cool;

Memmgr cgen_Memmgr = GC_NOGC;      // enable/disable garbage collection
Memmgr_Test cgen_Memmgr_Test = GC_NORMAL;  // normal/test GC
Memmgr_Debug cgen_Memmgr_Debug = GC_QUICK; // check heap frequently

bool cgen_optimize = false;       // optimize switch for code generator
bool disable_reg_alloc=false;     // Don't do register allocation




static const char *gc_init_names[] =
    { "_NoGC_Init", "_GenGC_Init", "_ScnGC_Init" };
static const char *gc_collect_names[] =
    { "_NoGC_Collect", "_GenGC_Collect", "_ScnGC_Collect" };


// The following temporary name will not conflict with any
// user-defined names.
#define TEMP1 "_1"


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

   //namespace {



   //} // anonymous namespace ???



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
//   	  MemoryLocation* offset = new IndirectLocation(next_offset, SELF);
		RegisterPointerOffset loc(SELF, next_offset);
  	  attrVarEnv_.Push(attr->name(), loc);  	  
  	  next_offset += WORD_SIZE; // pointers are 2 bytes
  	  objectSize_ += WORD_SIZE;	// update object size
  	}
  }
  
  for (CgenNode* child : children_)
  	{ child->CreateAttrVarEnv(next_offset); }
}


   /* dispatch tables output */
   std::ostream& operator<<(std::ostream& os, const DispatchEntry& dispent) {
      std::string full_method_label_str = std::string(dispent.klass_->value()) 
         + std::string(METHOD_SEP) + std::string(dispent.method_->value());
      std::string dispent_label_str = std::string(DISPENT_PREFIX) + full_method_label_str;
      LabelValue full_method_label(full_method_label_str);
      LabelValue dispent_label(dispent_label_str);
      
      /* define bcall handle */
      os << DEFINE << DISPENT_PREFIX << full_method_label << "\t" << "$-4000h" << std::endl;

      /* emit dispatch table entry data */
      os << DW << full_method_label << std::endl;         
      os << DB << dispent.page_ << std::endl;
      
      return os;
   }
   
   
   
   void DispatchTable::print_entries() const {
      for (auto p : *this) {
         const DispatchEntry& dispent = p.second;
         std::clog << "\t" << dispent.klass_->value() << METHOD_SEP << dispent.method_->value() 
                   << "\tpage " << dispent.page_ << "\t$" << std::hex
                   << dispent.addr_ << std::dec << std::endl;
      }
   }

   void DispatchTables::print_entries() const {
      for (auto p : *this) {
         Symbol *name = p.first;
         const DispatchTable& disptab = p.second;
         std::clog << name->value() << ":" << std::endl;
         disptab.print_entries();
      }
   }

// CreateDispatchTables
//  -creates a table of dispatch tables (one per class)
//   with each table mapping a method name to a memory location
   void CgenNode::CreateDispatchTables(DispatchTables& tables, 
                                       MethodInheritanceTable inheritance_t, int next_offset) {

      
      
      if (parent() != nullptr) {
         /* copy parent's table and modify absolute address in new disptab */
         Symbol *name = klass()->name();
         tables[name] = tables[parent()->name()];
         for (auto p : tables[name]) {
            DispatchEntry& dispent = p.second;
            dispent.loc_.label_ = std::string(name->value()) + std::string(DISPTAB_SUFFIX);
            tables[name][p.first] = dispent;
         }
      }
      
      /* add class's functions to inheritance table */
      for (auto features_it = klass()->features_begin(); features_it != klass()->features_end();
           features_it++) {
         if ((*features_it)->method()) {
            Method* method = (Method*) (*features_it);
            Symbol* name = method->name();
            inheritance_t[name] = klass()->name();
         }
      }
      
      /* add class's overridden/new methods to dispatch tables */
      for (Features::const_iterator feature = klass()->features_begin(); 
           feature != klass()->features_end(); ++feature) {
         if ((*feature)->method()) {
            Method* method = (Method*) (*feature);
            if (tables[klass()->name()].find(method->name()) == tables[klass()->name()].end()) {
               std::string dispTab_label = klass()->name()->value();
               dispTab_label.append(DISPTAB_SUFFIX);
               
               AbsoluteAddress loc(dispTab_label, next_offset);
               //AbsoluteAddress *loc = new AbsoluteAddress(dispTab_label, next_offset);
               Symbol *method_klassname = inheritance_t[method->name()];
               DispatchEntry entry(loc, 0, 0, method->name(), method_klassname);
               tables[klass()->name()][method->name()] = entry;
               next_offset += 1*WORD_SIZE + 1*BYTE_SIZE;
            } else {
               /* overridden method -- update DispatchEntry */
               DispatchEntry& dispent = tables[klass()->name()][method->name()];
               dispent.klass_ = this->klass()->name();
            }
         }
      }
      
      for (CgenNode* child : children_)
         { child->CreateDispatchTables(tables, inheritance_t, next_offset); }
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
  	isolated_node->tag_ = tag++;
    CgenNode* parent_node = ClassFind(isolated_node->parent_name());
    parent_node->children_.push_back(isolated_node);
    isolated_node->parent_ = parent_node;
  }
  
  // generate class variable environments, starting recursively from root (Object)
  root()->CreateAttrVarEnv(CgenLayout::Object::attribute_offset);

  // recursively generate dispatch tables, starting from empty table
  CgenNode::MethodInheritanceTable inheritance_t;
  root()->CreateDispatchTables(gCgenDispatchTables, inheritance_t, 0);
}



// 	TO BE CHANGED
void CgenKlassTable::CgenGlobalData(std::ostream& os) const {
  Symbol* main    = gIdentTable.emplace(MAINNAME);
  Symbol* string  = gIdentTable.emplace(STRINGNAME);
  Symbol* integer = gIdentTable.emplace(INTNAME);
  Symbol* boolc   = gIdentTable.emplace(BOOLNAME);

  // The following global names must be defined first.
//   os << GLOBAL << CLASSNAMETAB << std::endl;
//   os << GLOBAL; emit_protobj_ref(main, os);    os << std::endl;
//   os << GLOBAL; emit_protobj_ref(integer, os); os << std::endl;
//   os << GLOBAL; emit_protobj_ref(string, os);  os << std::endl;
//   os << GLOBAL << BOOLCONST_PREFIX << 0 << std::endl;
//   os << GLOBAL << BOOLCONST_PREFIX << 1 << std::endl;
//   os << GLOBAL << INTTAG << std::endl;
//   os << GLOBAL << BOOLTAG << std::endl;
//   os << GLOBAL << STRINGTAG << std::endl;

  // We also need to know the tag of the Int, String, and Bool classes
  // during code generation.
  os << INTTAG << LABEL << DW << TagFind(integer) << std::endl;
  os << BOOLTAG << LABEL << DW << TagFind(boolc) << std::endl;
  os << STRINGTAG << LABEL << DW <<  TagFind(string) << std::endl;
}

// void CgenKlassTable::CgenSelectGC(std::ostream& os) const {
// //   os << GLOBAL << "_MemMgr_INITIALIZER" << std::endl;
//   os << "_MemMgr_INITIALIZER:" << std::endl;
//   os << DW << gc_init_names[cgen_Memmgr] << std::endl;
// //   os << GLOBAL << "_MemMgr_COLLECTOR" << std::endl;
//   os << "_MemMgr_COLLECTOR:" << std::endl;
//   os << DW << gc_collect_names[cgen_Memmgr] << std::endl;
// //   os << GLOBAL << "_MemMgr_TEST" << std::endl;
//   os << "_MemMgr_TEST:" << std::endl;
//   os << DW << (cgen_Memmgr_Test == GC_TEST) << std::endl;
// }

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
//   os << GLOBAL << HEAP_START << std::endl
//       os << HEAP_START << LABEL
//       << WORD << 0 << std::endl
//       << "\t.text" << std::endl
//       << GLOBAL;
//   emit_init_ref(Main, os);
//   os << std::endl << GLOBAL;
//   emit_init_ref(Int, os);
//   os << std::endl << GLOBAL;
//   emit_init_ref(String, os);
//   os << std::endl << GLOBAL;
//   emit_init_ref(Bool, os);
//   os << std::endl << GLOBAL;
//   emit_method_ref(Main, main_meth, os);
//   os << std::endl;
}

// modified 8/2018
// NEW APPROACH: GENEREATE IN SEPARATE .asm FILE
void CgenNode::EmitDispatchTable(std::ostream& os, DispatchTables& tables, MethodInheritanceTable inheritance_t) {
  DispatchTable& table = tables[klass()->name()];
  
  std::vector<std::pair<Symbol*,DispatchEntry>> ordered_table;
  for (auto p : table) {
     ordered_table.push_back(p);
  }

	/* sort table by offsets of entries from dispatch table label */
	std::sort(ordered_table.begin(), ordered_table.end(), [](const auto& lhs, const auto& rhs){
      return lhs.second.loc_ < rhs.second.loc_; // compare absolute addresses
	});

	/* generate dispatch table for class */
  os << klass()->name() << DISPTAB_SUFFIX << LABEL;
  for (auto method_pair : ordered_table) {
     DispatchEntry& dispent = method_pair.second;
     os << dispent;
  }

  for (CgenNode* child : children_)
    { child->EmitDispatchTable(os, tables, inheritance_t); }
}

// CgenDispatchTables: emits all dispatch tables
void CgenKlassTable::CgenDispatchTables(std::ostream& os) const {
  CgenNode::MethodInheritanceTable inheritance_t;
  root()->EmitDispatchTable(os, gCgenDispatchTables, inheritance_t);
}

// modified 8/18
void CgenNode::EmitPrototypeObject(std::ostream& os) {
  // handle Int, String, Bool separately
  os << DW << "-1" << std::endl;	// GC tag
  os << klass()->name() << PROTOBJ_SUFFIX << LABEL; // protobj label
  os << DW << tag_ << std::endl; // class tag
  if (klass()->name() == String) {
    os << DW << 7 << std::endl; // size of object (bytes)
    os << DW << klass()->name() << DISPTAB_SUFFIX << std::endl;
    os << DW;
    CgenRef(os, gIntTable.lookup(0)) << std::endl;
    os << DW << 0 << std::endl;
  } else if (klass()->name() == Int || klass()->name() == Bool) {
    // attributes end up being the same for Int & Bool
    os << DW << 8 << std::endl; // 8 bytes
    os << DW << klass()->name() << DISPTAB_SUFFIX << std::endl;
    os << DW << 0 << std::endl;
  } else {
    os << DW << objectSize_ << std::endl;
    os << DW << klass()->name() << DISPTAB_SUFFIX << std::endl;
    for (int i = 0; i < attrVarEnv_.vars_.size(); ++i)
    	{ os << DW << 0 << std::endl; }
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
    bool had_string_entry = gStringTable.has(p.second->value());
    
    if (had_string_entry) { continue; } // skip if class' string already generated
    
    gStringTable.emplace(p.second->value());
    Symbol* class_name = gStringTable.lookup(p.second->value());
    CgenDef(os, class_name, TagFind(String));
    if (!had_length_entry) {
      // int constant for str len needs to be generated
      Int16Entry* length_entry = gIntTable.lookup(class_name->value().size());
      CgenDef(os, length_entry, TagFind(Int));
    }
  }
  
  os << CLASSNAMETAB << LABEL;
  for (std::pair<std::size_t,Symbol*> p : ordered_class_names) {
    os << DW;
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
    os << DW << p.second << PROTOBJ_SUFFIX << std::endl;
    os << DW << p.second << CLASSINIT_SUFFIX << std::endl;
  }
}


// EmitInitializer: emit initializer for class
void CgenNode::EmitInitializer(std::ostream& os) {
  attrVarEnv_.klass_ = klass(); // set current class
  attrVarEnv_.ResetTemporaryCount(); // so temporaries will be assigned to proper loc
  os << klass()->name() << CLASSINIT_SUFFIX << LABEL;

  if (klass()->name() == Object) {
  	// do nothing
  } else {
    Symbol* parent_name = parent()->klass()->name();
    
    // set up activation record
    emit_push(FP, os);
  	emit_push(SELF, os);
  	emit_load(RegisterValue(FP), Immediate16((int16_t) 0), os);
  	emit_add(FP, RegisterValue(SP), os);	// FP = new frame pointer value

  	// emit_load(SELF, ARG0, os); // bind self
    os << EX << rDE << "," << rHL << std::endl;
  emit_load(SELF, rDE, os); // bind self, but can only do it with DE -> IX
    
    
    // find maximum number of temporaries needed over all attributes
    int max_temps = 0;
    for (Features::const_iterator feature = klass()->features_begin(); feature != klass()->features_end(); ++feature) {
      if ((*feature)->attr()) {
        Attr* attr = (Attr*) (*feature);
        max_temps = std::max(max_temps, attr->init()->CalcTemps());
      }
    }
    assert (max_temps*WORD_SIZE == (int8_t) (max_temps*WORD_SIZE));	// if this fails, then codegen will fail -- need workaround
    // max_temps ≤ 63 because IY register can be offset using a signed 8-bit int, which allows for a range of -128≤x≤127 bytes
    emit_load(RegisterValue(rHL), Immediate16(static_cast<int16_t>(-max_temps*WORD_SIZE)), os);
    emit_add(rHL, RegisterValue(SP), os);
	emit_load(RegisterValue(SP), RegisterValue(rHL), os);
	// don't change IY, though
	
    // call parent initializer
//     emit_load(ARG0, SELF, os);
  	emit_load(rDE, SELF, os); // bind self, but can only do it with DE -> IX    
	os << EX << rDE << "," << rHL << std::endl;

    AbsoluteAddress addr(get_init_ref(parent_name));
    emit_call(addr, Flags::none, os);
	
	// set self for attribute initializers
// 	emit_load(SELF, ARG0, os); // dont need to do this, actually
	
	// initialize attributes
	for (Features::const_iterator feature = klass()->features_begin(); feature != klass()->features_end(); ++feature) {
	  if ((*feature)->attr()) {
	    Attr* attr = (Attr*) (*feature);
	    Expression* init = attr->init();
	    attrVarEnv_.init_type_ = attr->decl_type();    
	    init->CodeGen(attrVarEnv_, os);	// result in ACC
	    MemoryLocation& attr_offset = attrVarEnv_.Lookup(attr->name());   
		MemoryValue attr_val(attr_offset);
		emit_load(attr_val, ARG0, os);
	  }
	}
	
	// cleanup
	emit_load(rDE, SELF, os); 
	os << EX << rDE << "," << rHL << std::endl; // current object expected in ACC
	emit_load(SP, FP, os);
	emit_pop(SELF, os);
	emit_pop(FP, os);
		
	attrVarEnv_.init_type_ = nullptr;
  }
  emit_return(Flags::none, os); // return for both Object & other types
  
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


void CgenNode::EmitInheritanceInfo(std::ostream& os) const {
	os << DW << tag_ << std::endl;
	os << DW << (parent_->tag_ - tag_) * CgenLayout::InheritanceTree::entry_length - 1 << std::endl;
}

int CgenKlassTable::InheritanceDepth(const CgenNode *node) const {
	int depth;
	for (depth = 0; node && node != root(); ++depth, node = node->parent()) {}
	assert (node != nullptr);
	return depth;
}

std::pair<bool,int> CgenKlassTable::InheritanceDistance(const CgenNode* child, const CgenNode* parent) const {
	if (!(child && parent))
		return std::pair<bool,int>(false, 0);
	
	int dist = 0;
	const CgenNode* child_tmp = child;
	while (child_tmp != root() && child_tmp != parent) {
		child_tmp = child_tmp->parent();
		++dist;
	}
	if (child_tmp == parent)
		return std::pair<bool,int>(true, dist);
	
	// didn't find match, so consider case of 'parent' being child of 'child'
	dist = 0;
	const CgenNode* parent_tmp = parent;
	while (parent_tmp != root() && parent_tmp != child) {
		parent_tmp = parent_tmp->parent();
		--dist;
	}
	if (parent_tmp == child)
		return std::pair<bool,int>(true, dist);
	
	return std::pair<bool,int>(false, 0);
}

std::map<std::size_t, const CgenNode *> CgenKlassTable::GetTags() const {
	std::map<std::size_t, const CgenNode *> tags;
	std::deque<const CgenNode *> todo;
	todo.push_front(root());
	
	while (!todo.empty()) {
		const CgenNode *node = todo.back();
		todo.pop_back();
		tags[node->tag()] = node;
		std::vector<CgenNode*> children = node->children_;
		for (CgenNode *child : children)
			todo.push_front(child);
	}
	
	return tags;
}

// CgenClassMethods: emit methods for all classes
void CgenKlassTable::CgenClassMethods(std::ostream& os) const {
  root()->EmitMethods(os);
}

void CgenKlassTable::CgenInheritanceTree(std::ostream& os) const {
	os << INHERITANCE_TREE << LABEL;

	std::map<std::size_t, const CgenNode *> tags = GetTags();
	for (std::pair<std::size_t, const CgenNode *> tag : tags) {
		tag.second->EmitInheritanceInfo(os);
	}
		
}

void CgenHeader(std::ostream& os) {
	/* os << ".org $9D93" << std::endl;
    * os << ".db $BB,$6D ; AsmPrgm" << std::endl;
    * os << std::endl;
    */
	
	std::string inc_files[] = {
		"ti83plus.inc",
		"cool.inc",
		"app.inc"
	};

	std::string lib_files[] = {
		"boot.z80",
		"memory.z80",
		"display.z80",
		"keyboard.z80",
		"misc.z80",
		"Object.z80",
		"IO.z80",
		"math.z80",
		"String.z80"
	};

   std::string aux_files[] = {
      DISPTAB_PATH
   };

   /* include .inc files */
	for (std::string file : inc_files) {
		emit_include(file, os);
	}
		
   /* define first page */
	os << "defpage(0)" << std::endl;
	os << JP << "_start" << std::endl;

   /* include library files */
	for (std::string file : lib_files) {
		emit_include(file, os);
	}

   /* include auxiliary files */
   for (std::string file : aux_files) {
      emit_include(file, os);
   }
}

   void CgenKlassTable::CodeGen(std::ostream& os) const {
      CgenHeader(os);
      
      CgenGlobalData(os);
      //  CgenSelectGC(os);
      CgenConstants(os);
      
      CgenPrototypeObjects(os);  
      
      CgenClassObjTab(os);
      CgenClassNameTab(os);
      CgenInheritanceTree(os);
      
      CgenGlobalText(os);
      
  
      CgenClassInits(os);
      CgenClassMethods(os);

  /* generate dispatch tables to separate file */
  std::filebuf disptab_fb;
  if (disptab_fb.open(DISPTAB_PATH, std::ios::out) == NULL) {
     perror("std::filebuf.open");
     throw std::exception();
  }
  std::ostream disptab_os(&disptab_fb);
  CgenDispatchTables(disptab_os);
  disptab_os.flush();
  disptab_fb.close();
}



/* CODE GENERATION FOR AST NODES */
void Method::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  // need to add formals to variable environment
  // don't need to worry about binding self in codegen
  varEnv.ResetTemporaryCount();
  int temp_count = body_->CalcTemps();
  
  // perform callee AR setup
  emit_push(FP, os);
  emit_push(SELF, os);
  // note: FP is below (on top of) all temporaries on the stack, since IY can only be indexed with positive offsets
  emit_load(RegisterValue(FP), Immediate16(static_cast<int16_t>(-temp_count*WORD_SIZE)), os);
  emit_add(FP, SP, os);	// FP = new frame pointer value
  emit_load(SP, FP, os); // update stack pointer to end of temporaries
  
  os << EX << rDE << "," << rHL << std::endl;
  emit_load(RegisterValue(SELF), RegisterValue(rDE), os); // bind self, but can only do it with DE -> IX

  int formals_counter = formals()->size();
  for (Formals::const_iterator formals_it = formals_begin(); formals_it != formals_end(); ++formals_it) {
    Formal* formal = *formals_it;
    --formals_counter; // subtract first, since formals_counter starts out at 1 past last arg
    // need to assign location relative to FP
//     MemoryLocation* formal_loc = new IndirectLocation(formals_counter, FP);
	RegisterPointerOffset formal_loc(FP, (temp_count+formals_counter)*WORD_SIZE + CgenLayout::ActivationRecord::arguments_end);
    varEnv.Push(formal->name(), formal_loc);
  }
  
  body_->CodeGen(varEnv, os); // generate method body
  //int temporary_offset = varEnv.GetTemporaryMaxCount() * WORD_SIZE; // not sure why this was being used; redundant
  
  // pop entire AR off stack, NOT INCLUDING return addr. & arguments from caller
  os << EX << rDE << "," << rHL << std::endl; // preserve return value
  
  // not sure why temporary_offset was being used instead of temp_count...
  //emit_load(RegisterValue(rHL), Immediate16(static_cast<int16_t>(temporary_offset * WORD_SIZE)), os);
  emit_load(RegisterValue(rHL), Immediate16(static_cast<int16_t>(temp_count*WORD_SIZE)), os);
  
  emit_add(rHL, RegisterValue(SP), os);
  emit_load(RegisterValue(SP), RegisterValue(rHL), os);
  os << EX << rDE << "," << rHL << std::endl;
  emit_pop(SELF, os);
  emit_pop(FP, os);
  
  emit_return(Flags::none, os);
  
  // exit method scope
  for (Formals::const_iterator formals_it = formals_begin(); formals_it != formals_end(); ++formals_it) {
    Formal* formal = *formals_it;
    varEnv.Pop(formal->name());
  }
}

void BoolLiteral::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
	const char *addr = CgenRef(value()).c_str();
	const LabelValue lbl(addr);
	emit_load(RegisterValue(ARG0), lbl, os);
}

void IntLiteral::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  emit_load(RegisterValue(ARG0), CgenRef(value_), os);
}

void StringLiteral::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  emit_load(RegisterValue(ARG0), CgenRef(value_), os);
}

void NoExpr::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  // need to know type of relevant variable
  Symbol* init_t = varEnv.init_type_;
  if (init_t == Int) {
  	emit_load(RegisterValue(ARG0), CgenRef(gIntTable.lookup(0)), os);
  } else if (init_t == Bool) {
    emit_load(RegisterValue(ARG0), CgenRef(false), os);
  } else if (init_t == String) {
    emit_load(RegisterValue(ARG0), CgenRef(gStringTable.lookup(std::string(""))), os);
  } else {
    emit_load(RegisterValue(ARG0), Immediate16(static_cast<int16_t>(0)), os);	// Void pointer is default initialization
  }
}

void Ref::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  if (name_ == self) {
  	// this extra step is necessary -- ld h,ixh isn't allowed
    emit_load(rDE, SELF, os);
    os << EX << rDE << "," << rHL << std::endl;
  } else {
  	const MemoryLocation& loc = varEnv.Lookup(name_);
  	if (loc.kind() == MemoryLocation::Kind::ABS) {
  		emit_load(ARG0, loc, os);
  	} else if (loc.kind() == MemoryLocation::Kind::PTR) {
  		const RegisterPointer& ptr = (const RegisterPointer&) loc;
  		emit_load(ARG0.low(), ptr, os);
  		emit_inc(ptr.reg(), os);
  		emit_load(ARG0.high(), ptr, os);
  		emit_dec(ptr.reg(), os);
  	} else if (loc.kind() == MemoryLocation::Kind::PTR_OFF) {
  		// really, only this case should be called
  		const RegisterPointerOffset& ptr_off = (const RegisterPointerOffset&) loc;
  		emit_load(ARG0.low(), MemoryValue(ptr_off[0]), os);
  		emit_load(ARG0.high(), MemoryValue(ptr_off[1]), os);
  	} else {
  		assert (false);
  	}
  }
}

void BinaryOperator::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  // is the resulting object a Bool? (otherwise an Int)
//   if (lhs_->type() == Int) {
  	if (type() == Int) {
	  	// if result is Int, create new int object
  		emit_load(RegisterValue(ARG0), LabelValue(std::string(INTNAME) + std::string(PROTOBJ_SUFFIX)), os);
  		emit_copy(os);
		emit_push(ARG0, os);
  	}
	
  	lhs_->CodeGen(varEnv, os);
  	emit_push(ARG0, os);
  	rhs_->CodeGen(varEnv, os);
  	emit_pop(rDE, os);
  	
  	if (lhs_->type() == Int) {
  		// if LHS & RHS are Ints
  		emit_fetch_int(RegisterValue(rBC), RegisterPointer(rHL), os);
  		os << EX << rDE << "," << rHL << std::endl;
  		emit_fetch_int(RegisterValue(rDE), RegisterPointer(rHL), os);
  		os << EX << rDE << "," << rHL << std::endl;
  	} else if (lhs_->type() == Bool) {
  		// if LHS & RHS are bools
  		emit_fetch_bool(RegisterValue(rBC), RegisterPointer(rHL), os);
  		os << EX << rDE << "," << rHL << std::endl;
  		emit_fetch_bool(RegisterValue(rDE), RegisterPointer(rHL), os);
  		os << EX << rDE << "," << rHL << std::endl;
  	} else {
  		// else operands are objects
  		os << EX << rDE << "," << rHL << std::endl;
  		emit_load(rB, rD, os);
  		emit_load(rC, rE, os);
  	}
  	
  	// rHL = lhs, rBC = rhs
  	const int l_true = label_counter++;
  	const int l_end = label_counter++;
  	
  	switch (kind_) {
  	case BO_Add:
  		emit_add(rHL, rBC, os);
  		break;
  	case BO_Sub:
  		// need to negate rBC
  		emit_cpl(rBC, os);
		os << SCF << std::endl;
		emit_adc(rHL, rBC, os);
  		break;
  	case BO_Mul:
  		{
  		emit_load(rDE, rBC, os);
  		emit_call(lib::MUL_HL_DE, nullptr, os);
		break;
		}
  	case BO_Div:
  		{
  		// fast division (not signed yet...)
  		emit_load(rD, rB, os);
  		emit_load(rE, rC, os);// LD de,bc
  		emit_call(lib::DIV_HL_DE, nullptr, os);
  		break;
  		}
	case BO_LT:
		os << XOR << ACC << std::endl;
		os << SBC << rHL << "," << rBC << std::endl;
		os << ADD << rHL << "," << rHL << std::endl;
		emit_load(RegisterValue(rHL), CgenRef(true), os);
		emit_jr(l_true, Flags::C, os); // carry flag is set iff _lhs_ - _rhs_ < 0
		emit_load(RegisterValue(rHL), CgenRef(false), os);
		emit_label_def(l_true, os);
		break;
	case BO_LE:
		os << SCF << std::endl;
		os << SBC << rHL << "," << rBC << std::endl;
		os << ADD << rHL << "," << rHL << std::endl;
		emit_load(RegisterValue(rHL), CgenRef(true), os);
		emit_jr(l_true, Flags::C, os);
		emit_load(RegisterValue(rHL), CgenRef(false), os);
		emit_label_def(l_true, os);
		break;
		
	case BO_EQ:
		os << XOR << rA << std::endl;
		os << SBC << rHL << "," << rBC << std::endl;
		emit_load(RegisterValue(rHL), CgenRef(true), os);
		emit_jr(l_end, Flags::Z, os);
		emit_load(RegisterValue(rHL), CgenRef(false), os);
		emit_label_def(l_end, os);
		break;
	
  	default:
  		assert (false);
  	}
  	
  	if (type() == Int) {
  		os << EX << "de,hl" << std::endl; // exchange values
  		emit_pop(ARG0, os); // pop off copied protoype int obj
  		const RegisterPointer new_int(ARG0);
  		emit_store_int(rDE, new_int, os);
  	} else {
  		assert (type() == Bool);
  		// ARG0 already equals bool
  	}
}

void UnaryOperator::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
	const int l_end = label_counter++;

  switch (kind_) {
  case UO_Neg:
   {
  	assert (input_->type() == Int);
  	
  	const std::string protobj(std::string(INTNAME) + std::string(PROTOBJ_SUFFIX));
  	emit_load(RegisterValue(ARG0), LabelValue(protobj), os);
  	emit_copy(os);
  	emit_push(ARG0, os);
  	
  	input_->CodeGen(varEnv, os);
  	
  	emit_fetch_int(rDE, RegisterPointer(ARG0), os);
  	emit_load(RegisterValue(rHL), Immediate16(static_cast<int16_t>(0)), os);
	os << XOR << rA << std::endl;
	os << SBC << rHL << "," << rDE << std::endl;
	os << EX << rDE << "," << rHL << std::endl;
	emit_pop(ARG0, os);
	emit_store_int(rDE, RegisterPointer(ARG0), os);
	break;
  	}
  case UO_Not:
  	assert (input_->type() == Bool);
  	input_->CodeGen(varEnv, os);
  	emit_fetch_bool(rDE, RegisterPointer(ARG0), os);
  	os << LD << rA << "," << rD << std::endl;
  	os << OR << rE << std::endl;
  	emit_load(RegisterValue(ARG0), CgenRef(false), os);
  	emit_jr(l_end, Flags::NZ, os);
  	emit_load(RegisterValue(ARG0), CgenRef(true), os);
  	emit_label_def(l_end, os);
  	break;
  
  case UO_IsVoid:  	
  	input_->CodeGen(varEnv, os);
  	os << LD << rA << "," << rH << std::endl;
  	os << OR << rL << std::endl;
  	emit_load(RegisterValue(ARG0), CgenRef(false), os);
  	emit_jr(l_end, Flags::NZ, os);
  	emit_load(RegisterValue(ARG0), CgenRef(true), os);
  	emit_label_def(l_end, os);
  	break;
  
  default:
  	assert (false);
  }
  
}

void Knew::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  if (name_ == SELF_TYPE) {
  	// retrieve class tag,
  	// lookup location of prototype object,
  	// copy object
  	const int l_ret = label_counter++;
  	const RegisterPointerOffset tag_loc(SELF, TAG_OFFSET);
  	emit_load(ARG0, tag_loc, os);
  	emit_add(ARG0, ARG0, os);
  	emit_add(ARG0, ARG0, os);
  	emit_load(RegisterValue(rDE), LabelValue(CLASSOBJTAB), os);
  	emit_add(ARG0, rDE, os);	// (ARG0) = protobj
  	emit_load(rDE, RegisterPointer(ARG0), os);
  	os << EX << rDE << "," << rHL << std::endl;
  	emit_push(rDE, os); // save pointer to protobj
  	emit_copy(os);
  	emit_pop(rDE, os);
  	emit_inc(rDE, os);
  	emit_inc(rDE, os); // (de) = init
  	os << EX << rDE << "," << rHL << std::endl;
  	emit_load(rBC, RegisterPointer(ARG0), os);
  	emit_load(RegisterValue(ARG0), LabelValue(label_ref(l_ret)), os);
  	emit_push(ARG0, os);
  	emit_push(rBC, os); // init method
  	os << EX << rDE << "," << rHL << std::endl; // HL = new obj
  	emit_return(nullptr, os); // hacky function call equivalent
  } else {
  	const std::string prot = std::string(name_->value()) + std::string(PROTOBJ_SUFFIX);
  	emit_load(RegisterValue(ARG0), LabelValue(prot), os);
  	emit_copy(os);
  	emit_init(name_, os);  
  }
}

void KaseBranch::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  // need to store location
  RegisterPointerOffset branch_var_loc(FP, varEnv.GetTemporaryCount() * WORD_SIZE);
  varEnv.Push(name_, branch_var_loc);
  varEnv.IncTemporaryCount();
  
  // expect address of evaluated objetc expr0 to be held on top of stack
 // emit_load(branch_var_loc, rBC, os);	// save expr0 object
	emit_pop(rDE, os);
	emit_load(branch_var_loc, rDE, os);
  
  body_->CodeGen(varEnv, os);
  
  varEnv.Pop(name_);
  varEnv.DecTemporaryCount();
}

void Kase::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
	const CgenNode *input_node;
	std::unordered_map<const KaseBranch *, const CgenNode *> branch2node;
	std::vector<KaseBranch *> branches;
	std::unordered_map<KaseBranch *, int> branch2label;
	
	input_node = gCgenKlassTable->ClassFind(input_->type());
	
	// construct KaseBranch-to-CgenNode table and branches vector
	for (KaseBranch *branch : *cases_) {
		branch2node[branch] = gCgenKlassTable->ClassFind(branch->decl_type());
		branch2label[branch] = label_counter++;
		branches.push_back(branch);
	}

	// comparator for sorting branches
	auto sort_branches = [&](const KaseBranch *lhs, const KaseBranch *rhs) {
		int left_depth = gCgenKlassTable->InheritanceDepth(branch2node[lhs]);
		int right_depth = gCgenKlassTable->InheritanceDepth(branch2node[rhs]);
		
		return left_depth > right_depth;
	};
	
	// sort the branches
	std::sort(branches.begin(), branches.end(), sort_branches);

	//-- ASSEMBLY CODE GENERATION STARTS HERE --//
	int abort_label = label_counter++;
	int end_label = label_counter++;
	
	// evaluate input expression
	input_->CodeGen(varEnv, os);
	emit_push(ARG0, os); // preserve address of input object
	
	// set rBC = tag of input object in rHL
	emit_load(RegisterValue(rBC), RegisterPointer(ARG0), os);
	
	// generate inheritance search code for each branch
	for (KaseBranch *branch : branches) {
		int branch_tag = branch2node[branch]->tag();
		int16_t tree_offset = CgenLayout::InheritanceTree::entry_length * branch_tag;
		os << LD << rHL << "," << INHERITANCE_TREE << "+" << tree_offset << std::endl;
		
		// inheritance tree traversal loop
		int loop_label = label_counter++;
		int branch_label = branch2label[branch];
		emit_label_def(loop_label, os);
			emit_load(RegisterValue(rDE), RegisterPointer(rHL), os);
			os << EX << rDE << "," << rHL << std::endl;
			os << XOR << rA << std::endl;
			os << SBC << rHL << "," << rBC << std::endl; // compare tags
			emit_jp(branch_label, Flags::Z, os); // if tags equal, go to branch
			
			// otherwise, compute parent node
			os << EX << rDE << "," << rHL << std::endl;
			os << INC << rHL << std::endl;
			os << INC << rHL << std::endl; // rHL points to offset in tree
			emit_load(RegisterValue(rDE), RegisterPointer(rHL), os);
			emit_add(RegisterValue(rHL), RegisterValue(rDE), os);
						
			// test if offset is 0
			os << LD << rA << "," << rD << std::endl;
			os << OR << rE << std::endl;
			emit_jr(loop_label, Flags::NZ, os);
			
			// next case follows
	}
	
	emit_label_def(abort_label, os);
		os << BREAK << std::endl; // TO BE IMPLEMENTED
		
		
	// codegen branches
	for (KaseBranch *branch : branches) {
		emit_label_def(branch2label[branch], os);
		branch->CodeGen(varEnv, os);
		emit_jp(end_label, nullptr, os);
	}
	
	emit_label_def(end_label, os);
	
}

/*
void Kase::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  const int case_loop = label_counter++;
  const int case_loop_continue = label_counter++;
  const int case_loop2 = label_counter++;
  const int case_loop2_continue = label_counter++;
  const int case_found = label_counter++;
  const int case_default = label_counter++;
  const int case_abort2 = label_counter++;
  const int case_table = label_counter++;
  const int case_end = label_counter++;
  
  const int case_table_entry_size = 2*WORD_SIZE;

  input_->CodeGen(varEnv, os);  // evaluate case expr
  
  os << XOR << ACC << std::endl;
  os << OR << rH << std::endl;
  os << OR << rL << std::endl;
  emit_jp(case_abort2, Flags::Z, os); // case abort 2 -- expr is void
  
  emit_push(rHL, os); // preserve result of expr0
  emit_load(RegisterValue(rDE), Immediate16(static_cast<int16_t>(TAG_OFFSET*WORD_SIZE)), os);
  emit_add(ARG0, RegisterValue(rDE), os);
  emit_load(RegisterValue(rDE), RegisterPointer(ARG0), os);
  // de = tag of expr0
  emit_load(RegisterValue(rHL), LabelValue(INHERITANCE_TREE), os);
  
  os << EX << rDE << "," << rHL << std::endl;
  emit_add(rHL, rHL, os); // faster than bit shifting
  emit_add(rHL, rDE, os);
  // hl = beginning of typeid's entry in inheritance table  
  
  // now, find optimal branch type match search order
  	const CgenNode *exp0_node = gCgenKlassTable->ClassFind(input_->type());
  	
  	std::clog << "exp0_node=" << exp0_node->tag() << std::endl;
  	
	std::vector<const CgenNode *> subclass_nodes, superclass_nodes;
	std::unordered_map<const CgenNode *, const KaseBranch *> subclass_map, superclass_map;
	for (const KaseBranch *branch : *cases_) {
		const CgenNode *node = gCgenKlassTable->ClassFind(branch->decl_type());
		if (node <= exp0_node) {
			subclass_nodes.push_back(node);
			subclass_map[node] = branch;
		} else if (exp0_node >= node) {
			superclass_nodes.push_back(node);
			superclass_map[node] = branch;
		}
	}
	
	std::clog << "subclass_nodes=" << subclass_nodes.size() << std::endl;
	std::clog << "superclass_nodes=" << superclass_nodes.size() << std::endl;
	
	auto sort_nodes = [&](const CgenNode *lhs, const CgenNode *rhs) {
		std::pair<bool,int> leftchild = gCgenKlassTable->InheritanceDistance(lhs, exp0_node);
		std::pair<bool,int> rightchild = gCgenKlassTable->InheritanceDistance(rhs, exp0_node);
		assert (leftchild.first && rightchild.first);
		return leftchild.second < rightchild.second;
	};
	
	// sort nodes based on inheritance distance
	std::sort(subclass_nodes.begin(), subclass_nodes.end(), sort_nodes);
	const CgenNode *super_node;
	const KaseBranch *super_branch;
	if (superclass_nodes.size() > 0) {
		super_node = *std::min_element(superclass_nodes.begin(), superclass_nodes.end(), sort_nodes);
		super_branch = superclass_map[super_node];
	} else {
		super_node = nullptr;
		super_branch = nullptr;
	}
	
	std::unordered_map<const CgenNode *, int> branch_labels;
	for (const CgenNode *node : subclass_nodes) {
		branch_labels[node] = label_counter++;
	}
	branch_labels[super_node] = label_counter++;
  
  	  // now, search for closest matching branch
	// remember hl = expr0's typeid entry address in inheritance table
	const Register16* rCASETAB = &rHL; // de holds current case/branch table address
	const Register16* rTYPETAB = &rDE; // hl holds current inheritance table address
	const Register16* rCASEID = &rBC; // bc holds typeid in tree TYPEID = (TYPETAB)
		
	emit_load(RegisterValue(*rCASETAB), LabelValue(label_ref(case_table)), os);
	emit_label_def(case_loop, os);
		emit_load(*rCASEID, RegisterPointer(*rCASETAB), os); // bc = type tag for current branch
		os << BIT << 7 << "," << rCASEID->high() << std::endl;
		emit_jr(case_default, Flags::NZ, os);// if caseid < 0, then end of table has been reached
		emit_push(*rTYPETAB, os); // preserve address of expr0 in inheritance table
		os << EX << *rTYPETAB << "," << *rCASETAB << std::endl;
		std::swap(rCASETAB, rTYPETAB);
				
		// only have to check up to the statically inferred class of expr0
		// inner loop to compare	
		// within inner loop, rTYPETAB = hl, rCASETAB = de	
		emit_label_def(case_loop2, os);			
			emit_load(ACC, rCASEID->low(), os);
			emit_cp(RegisterPointer(*rTYPETAB), os);
			emit_jr(case_loop2_continue, Flags::NZ, os);
			emit_load(ACC, rCASEID->high(), os);
			emit_inc(*rTYPETAB, os);
			emit_cp(RegisterPointer(*rTYPETAB), os);
			emit_jr(case_found, Flags::Z, os);
		emit_label_def(case_loop2_continue, os);
			emit_inc(*rTYPETAB, os);
			// rTYPETAB points to relative offset of parent
			emit_push(rBC, os);
			emit_load(rBC, RegisterPointer(*rTYPETAB), os);
			os << XOR << ACC << std::endl;
			emit_or(rB, os);
			emit_or(rC, os);
			emit_add(*rTYPETAB, rBC, os); // this doesn't affect Z flag, thankfully
			emit_pop(rBC, os);
			emit_jr(case_loop2, Flags::NZ, os);
// 			emit_jr(LabelValue(case_loop_continue), Flags::Z, os); // if bc = 0, then reached root node			
			///////////
	emit_label_def(case_loop_continue, os);
		os << EX << *rCASETAB << "," << *rTYPETAB << std::endl;
		std::swap(rCASETAB, rTYPETAB);
		
		emit_pop(*rTYPETAB, os);
		for (int i = 0; i < case_table_entry_size; ++i) {
			emit_inc(*rCASETAB, os);
		}
		emit_jr(case_loop, Flags::none, os);
			
	emit_label_def(case_found, os);
	std::swap(rCASETAB, rTYPETAB);
		os << EX << rDE << "," << rHL << std::endl;
		std::swap(rCASETAB, rTYPETAB);
		emit_pop(rBC, os); // rBC won't be used, but need to pop off temp value from case_loop
		for (int i = 0; i < WORD_SIZE; ++i) {
			emit_inc(*rCASETAB, os);
		}
		// (rCASETAB) = address of branch
		emit_load(rDE, RegisterPointer(*rCASETAB), os);
		emit_pop(rBC, os); // rBC = expr0
		os << EX << rDE << "," << rHL << std::endl;
		std::swap(rCASETAB, rTYPETAB);
		emit_load(RegisterValue(rDE), LabelValue(label_ref(case_end)), os);
		emit_push(rDE, os); // return address
		emit_jp(RegisterPointer(rHL), Flags::none, os);	// jump to corresponding branch
	std::swap(rCASETAB, rTYPETAB);
		
	emit_label_def(case_default, os); // no matching branch, so check for superclass
		if (super_node) {
			// since super node present, this is default case.
			// simply call this branch
			emit_pop(rBC, os); // rBC = expr0
			const AbsoluteAddress addr(label_ref(branch_labels[super_node]));
			emit_call(addr, Flags::none, os);
			emit_jr(case_end, Flags::none, os);
		} else {
			// throw case_abort error.
			// implement l8ter
			emit_pop(ARG0, os); // _case_abort needs this
			const AbsoluteAddress addr("_case_abort");
			emit_jp(addr, Flags::none, os);
		}
	
	emit_label_def(case_abort2, os);
		// expr0 is void
		os << LD << ARG0 << "," << CgenRef(gStringTable.lookup(varEnv.klass_->filename()->value())) << std::endl;
		os << LD << rDE << "," << this->loc() << std::endl;
		const AbsoluteAddress addr("_case_abort2");
		emit_jp(addr, Flags::none, os);
		

	// generate case branch table
	// contents of each entry:
	//    * type id (2 bytes)
	//    * address of branch (2 bytes)
	emit_label_def(case_table, os);
	
	for (const CgenNode *node : subclass_nodes) {
		const CgenNode::ClassTag tag = node->tag();
		os << DW << tag << std::endl;
		os << DW << label_ref(branch_labels[node]) << std::endl;
	}
	os << DW << -1 << std::endl; // marks end of table;

	for (KaseBranch *branch : *cases_) {
		const CgenNode *node = gCgenKlassTable->ClassFind(branch->decl_type());
		if (node) {
			emit_label_def(branch_labels[node], os);
			branch->CodeGen(varEnv, os);
			emit_jp(case_end, nullptr, os);
		}
	}

	emit_label_def(case_end, os);
}
*/

void Let::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  // SELF_TYPE not allowed, so don't need to consider
  RegisterPointerOffset let_var(FP, varEnv.GetTemporaryCount()*WORD_SIZE);
	varEnv.IncTemporaryCount();
  varEnv.init_type_ = decl_type_;
  
  init_->CodeGen(varEnv, os);
  varEnv.Push(name_, let_var);
  emit_load(let_var, ARG0, os);
  
  body_->CodeGen(varEnv, os);
  
  varEnv.Pop(name_);
  varEnv.init_type_ = nullptr;
  varEnv.DecTemporaryCount();
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
  emit_fetch_bool(RegisterValue(rBC), RegisterPointer(ARG0), os);
//   emit_beqz(ACC, label_loop_end, os);
  os << XOR << ACC << std::endl;
  emit_or(rB, os);
  emit_or(rC, os);
  emit_jp(label_loop_end, Flags::Z, os);
  
  body_->CodeGen(varEnv, os);
  emit_jp(label_loop_pred, nullptr, os);
  
  emit_label_def(label_loop_end, os);	// loop done
  emit_load(RegisterValue(ARG0), Immediate16(static_cast<int16_t>(0)), os); // loop evaluates to void
}

void Cond::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  int label_else = label_counter++;
  int label_fi = label_counter++;
  
  // evaluate predicate
  pred_->CodeGen(varEnv, os); // if
  emit_fetch_bool(RegisterValue(rDE), RegisterPointer(ARG0), os); // get bool value
  os << XOR << ACC << std::endl;
  emit_or(rD, os);
  emit_or(rE, os);
  emit_jp(label_else, Flags::Z, os);
//   emit_beqz(ACC, label_else, os); // branch to 'else' if false
  
  then_branch_->CodeGen(varEnv, os); // then
//   emit_branch(label_fi, os);
	emit_jp(label_fi, nullptr, os);
  
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
  
//   emit_push(RA, os);
  for (Expression* expr : *actuals_) {
    expr->CodeGen(varEnv, os);
    emit_push(ARG0, os);
  }

  receiver_->CodeGen(varEnv, os);
  os << XOR << ACC << std::endl;
  emit_or(rH, os);
  emit_or(rL, os);
  emit_jr(dispatch_abort, Flags::Z, os); // abort if receiver is void
  
  // call static method on given class
  //os << CALL << dispatch_type_ << METHOD_SEP << name_ << std::endl;
  std::string dispatch_str = std::string(DISPENT_PREFIX) + dispatch_type_->value() 
     + std::string(METHOD_SEP) + name_->value();
  AbsoluteAddress dispatch_addr(dispatch_str);
  emit_bcall(dispatch_addr, os); // bcall method

  
  for (Expression *expr : *actuals_) {
     emit_pop(rDE, os); // pop off args
  }
  
  emit_jr(dispatch_end, nullptr, os);

	emit_label_def(dispatch_abort, os);
	emit_load(RegisterValue(rDE), Immediate16(static_cast<uint16_t>(this->loc())), os);
	emit_load(RegisterValue(ARG0), CgenRef(gStringTable.lookup(varEnv.klass_->filename()->value())), os);
	const AbsoluteAddress disp_abort("_dispatch_abort");
	emit_jp(disp_abort, Flags::none, os);

	emit_label_def(dispatch_end, os);
}

void Dispatch::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  const int dispatch_end = label_counter++;
  const int dispatch_abort = label_counter++;

  for (Expression* expr : *actuals_) {
    expr->CodeGen(varEnv, os);
    emit_push(ARG0, os);
  }
  
  receiver_->CodeGen(varEnv, os);
  os << XOR << ACC << std::endl;
  emit_or(rH, os);
  emit_or(rL, os);
  emit_jr(dispatch_abort, Flags::Z, os); // if receiver is void, call dispatch_abort
  
  emit_load(RegisterValue(rDE), Immediate16(static_cast<int16_t>(DISPTABLE_OFFSET * WORD_SIZE)),
            os);
  os << EX << rDE << "," << rHL << std::endl;
  emit_add(ARG0, rDE, os); // ARG0 -> pointer to disptable for class
  emit_load(RegisterValue(rBC), RegisterPointer(ARG0), os); // rBC = address of disptable
  
  int16_t method_offset;
  if (receiver_->type() == SELF_TYPE) {
     method_offset = gCgenDispatchTables[varEnv.klass_->name()][name_].loc_.offset();
  } else {
     method_offset = gCgenDispatchTables[receiver_->type()][name_].loc_.offset();
  }
  
  emit_load(RegisterValue(ARG0), Immediate16(method_offset), os);
  emit_add(ARG0, rBC, os); // ARG0 = pointer to address of function to call
  emit_load(RegisterValue(rBC), RegisterPointer(rHL), os); // rBC = address of funciton to call
  
  os << EX << rDE << "," << rHL << std::endl;
  // rHL = receiver
  emit_load(RegisterValue(rDE), LabelValue(label_ref(dispatch_end)), os); // push return address
  emit_push(rDE, os);
  
  emit_push(rBC, os);
  emit_return(Flags::none, os);     // jp (bc)
  
  emit_label_def(dispatch_abort, os); // if receiver is NULL
	emit_load(RegisterValue(rDE), Immediate16(static_cast<uint16_t>(this->loc())), os);
	emit_load(RegisterValue(ARG0), CgenRef(gStringTable.lookup(varEnv.klass_->filename()->value())), os);
	const AbsoluteAddress disp_abort("_dispatch_abort");
	emit_jp(disp_abort, Flags::none, os);

  
  emit_label_def(dispatch_end, os);
  // pop off params
  for (Expression *expr : *actuals_) {
  	emit_pop(rDE, os);
  }
}

void Assign::CodeGen(VariableEnvironment& varEnv, std::ostream& os) {
  // lookup memory location
  const MemoryLocation& loc = varEnv.Lookup(name_);
  value_->CodeGen(varEnv, os);
  emit_load(MemoryValue(loc), ARG0, os);
}


void Cgen(Program* program, std::ostream& os) {
  InitCoolSymbols();

  CgenKlassTable klass_table(program->klasses());
  gCgenKlassTable = &klass_table;
  klass_table.CodeGen(os);
}



}
