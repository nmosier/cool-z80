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

#ifndef __CGEN_H
#define __CGEN_H

#include <assert.h>
#include <stdio.h>
#include "emit.h"
#include "scopedtab.h"
#include "ast.h"
#include "ast_consumer.h"
#include "register.h"

#include <exception>
#include <list>
#include <set>
#include <map>

extern bool cgen_optimize;       // optimize switch for code generator
extern bool disable_reg_alloc;

//
// Garbage collection options
//

extern enum Memmgr { GC_NOGC, GC_GENGC, GC_SNCGC } cgen_Memmgr;

extern enum Memmgr_Test { GC_NORMAL, GC_TEST } cgen_Memmgr_Test;

extern enum Memmgr_Debug { GC_QUICK, GC_DEBUG } cgen_Memmgr_Debug;


namespace cool {

/**
 * Set to true to display debug output
 */
extern bool gCgenDebug;

struct CgenLayout {
	struct ActivationRecord {
		static const int8_t callee_add_size = -2 * WORD_SIZE;
		static const int8_t caller_self_offset = 0 * WORD_SIZE;
		static const int8_t caller_frame_offset = 1 * WORD_SIZE;
		static const int8_t return_value = 2 * WORD_SIZE;
		static const int8_t arguments_end = 3 * WORD_SIZE; // one past end
	};
   
	struct Object {
		static const int8_t attribute_offset = 3 * WORD_SIZE;
	};
	
	struct InheritanceTree {
		static const int16_t entry_length = 2 * WORD_SIZE;
	};
};

/**
 * Main entry point for code generation
 * @param program Program AST node
 * @param os std::ostream to write generated code to
 */
 void Cgen(Program* program, std::ostream& os, const char *asm_path, const char *lib_path);
 
 // Forward declarations
 class CgenKlassTable;
   class DispatchEntry;
   class DispatchTable;
   class DispatchTables;
   typedef std::unordered_map<std::string,unsigned int> AsmSymbolTable;

   class DispatchEntry {
   public:
      AbsoluteAddress loc_;
      unsigned int page_;
      unsigned int addr_;
      const Symbol *method_;
      const Symbol *klass_;
      
   DispatchEntry(): loc_(), page_(0), addr_(0), method_(NULL), klass_(NULL) {}
   DispatchEntry(const AbsoluteAddress& loc, const Symbol *method, const Symbol *klass):
      loc_(loc), page_(0), addr_(0), method_(method), klass_(klass) {}
      
   DispatchEntry(const AbsoluteAddress& loc, unsigned int page, unsigned int addr, 
                 const Symbol *method, const Symbol *klass):
      loc_(loc), page_(page), addr_(addr), method_(method), klass_(klass) {}

      void LoadDispatchSymbol(const AsmSymbolTable& symtab);
            
      friend std::ostream& operator<<(std::ostream& os, const DispatchEntry
                                      & dispent);
      
   };
   
   
   
   class DispatchTable: public std::unordered_map<Symbol*,DispatchEntry> {
   public:
      void print_entries() const;
      void LoadDispatchSymbols(const AsmSymbolTable& symtab);
   };
   
   class DispatchTables: public std::unordered_map<Symbol*,DispatchTable> {
   public: 
      void print_entries() const;
   };
 
 class VariableEnvironment {
 public:
 VariableEnvironment(Klass* klass): temporary_count_(0), temporary_max_count_(0), klass_(klass), init_type_(nullptr) {}
    
  void Push(Symbol* var, MemoryLocation& offset) { vars_[var].push_back(offset.copy()); }
  void Pop(Symbol* var) { vars_[var].pop_back(); }
  MemoryLocation& Lookup(Symbol* var) { return *vars_[var].back(); }	// returns offset  
  
  int GetTemporaryCount() { return temporary_count_; }
  int GetTemporaryMaxCount() { return temporary_max_count_; }
  int IncTemporaryCount() {
    temporary_max_count_ = std::max(++temporary_count_, temporary_max_count_);
    return temporary_count_;
  }
  int DecTemporaryCount() { return --temporary_count_; }
  int ResetTemporaryCount() { return (temporary_max_count_ = temporary_count_ = 0); }
  
  int temporary_count_;
  int temporary_max_count_;
  Klass* klass_;
  Symbol* init_type_; // only used for generating NoExpr's, but needs to be updated before every object initialization
  std::unordered_map<Symbol*,std::list<MemoryLocation*>> vars_;	// use list to encapsulate scopes 
};


/**
 * Node in the code generation inheritance graph
 *
 * There
 */
 class CgenNode : public InheritanceNode<CgenNode> {
 public:
    typedef std::int16_t ClassTag;
    typedef std::unordered_map<Symbol*,Symbol*> MethodInheritanceTable;
    
 CgenNode(Klass* klass, bool inheritable, bool basic) : InheritanceNode(klass, inheritable, basic), 
       tag_(0), attrVarEnv_(klass), objectSize_(3*WORD_SIZE) {}
    ~CgenNode() {}
  
  int16_t tag() const { return tag_; }
  DispatchTable& dispTab() { return dispTab_; } 
  
 private:
  /**
   * Unique integer tag for class.
   *
   * As specified in a "A Tour of the Cool Support Code", each class has a unique integer tag. "The runtime
   * system uses the class tag in equality comparisons between objects of the basic classes and in the abort
   * functions to index a table containing the name of each class."
   *
   * During construction this tag is set to 0. You will need to set the tag to a unique value at some point
   * during the initialization of the node. Other than that the tags are unique, there is no specific
   * ordering requirement.
   */
  ClassTag tag_;
  
  /* stores base variable environment, including only attributes
     to be used during recursive code generation
   */
  VariableEnvironment attrVarEnv_;
  int objectSize_;
  
  DispatchTable dispTab_;
  MethodInheritanceTable methodInheritanceTab_;

  void CreateAttrVarEnv(int next_offset);
  void CreateDispatchTables(const DispatchTable& parent_dispatch_table, 
                            const MethodInheritanceTable& parent_inheritance_table, int next_offset);
  void LoadDispatchSymbols(const AsmSymbolTable& symtab);
  
  void EmitPLT(std::ostream& os, MethodInheritanceTable inheritance_t);
  
  void EmitDispatchTable(std::ostream& os);
  
  void EmitPrototypeObject(std::ostream& os);
  
  void EmitInitializer(std::ostream& os);
  void EmitMethods(std::ostream& os);
  
  void EmitInheritanceInfo(std::ostream& os) const;
  
  friend class CgenKlassTable;
};


class CgenKlassTable : public KlassTable<CgenNode> {
 public:
  CgenKlassTable(Klasses* klasses);

  /**
   * Find integer tag for a class by name
   * @param name Class name
   * @return tag
   */
  CgenNode::ClassTag TagFind(Symbol* name) const {
    auto node = ClassFind(name);
    assert(node);
    return node->tag_;
  }
  
  std::pair<bool,int> InheritanceDistance(const CgenNode* node1, const CgenNode* node2) const;
  int InheritanceDepth(const CgenNode *node) const;

	std::map<std::size_t, const CgenNode *> GetTags() const;

  /**
   * Generate code for entire Cool program
   *
   * Main entry point for code generation
   *
   * @param os std::ostream to write generated code to
   */
   void CodeGen(std::ostream& os, const char *asm_path, const char *lib_path);

 private:
  /**
   * Symbol table (loaded after first pass of code generation).
   */
  AsmSymbolTable symtab_;

  /**
   * Emit code to the start the .data segment and declare global names
   * @param os std::ostream to write generated code to
   */
  void CgenGlobalData(std::ostream&) const;

  /**
   * Emit code to select the GC mode
   * @param os std::ostream to write generated code to
   */
//   void CgenSelectGC(std::ostream& os) const;

  /**
   * Emit constants (literals)
   * @param os std::ostream to write generated code to
   */
  void CgenConstants(std::ostream& os) const;


  /**
   * Emit code to start the .text segment and declare global names
   * @param os std::ostream to write generated code to
   */
  void CgenGlobalText(std::ostream& os) const;
  
  /**
   * Emit code for dispatch tables
   */
  void CgenDispatchTables(std::ostream& os) const;
  
  /**
   * Emit code for prototype objects (except for predefined classes)
   */
  void CgenPrototypeObjects(std::ostream& os) const;
  
  /**
   * Emit code for class_nameTab
   */
  void CgenClassNameTab(std::ostream& os) const;
  
  /**
   * Emit code ofr class_objTab
   */
  void CgenClassObjTab(std::ostream& os) const;
  
  /**
   * Emit code for class initializers
   */
  void CgenClassInits(std::ostream& os) const;
  
  /**
   * Emit code for class methods
   */
  void CgenClassMethods(std::ostream& os) const;
  
  /**
   * Emit inheritance tree for case expressions
   */
  void CgenInheritanceTree(std::ostream& os) const;

  /**
   * Emit symbol table for assembled program (first pass).
   */
  void CgenSymbolTable(const char *asm_path, const char *lib_dir);

};


} // namespace cool

#endif
