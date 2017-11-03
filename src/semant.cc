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

//   This file implements the semantic checks for Cool.  There are three
//   passes:
//
//   Pass 1: This is not a true pass, as only the classes are inspected.
//   The inheritance graph is built and checked for errors. There are
//   two "sub"-passes: check that classes are not redefined and inherit
//   only from defined classes, and check for cycles in the inheritance
//   graph. Compilation is halted if an error is detected between the
//   sub-passes.
//
//   Pass 2: Symbol tables are built for each class.  This step is done
//   separately because methods and attributes have global
//   scope; therefore, bindings for all methods and attributes must be
//   known before type checking can be done.
//
//   Pass 3: The inheritance graph (which is known to be a tree if
//   there are no cycles) is traversed again, starting from the root
//   class Object. For each class, each attribute and method is
//   typechecked. Simultaneously, identifiers are checked for correct
//   definition/use and for multiple definitions. An invariant is
//   maintained that all parents of a class are checked before a class
//   is checked.


#include <cstdlib>
#include <algorithm>
#include <set>

#include "semant.h"
#include "utilities.h"
#include "ast.h"

namespace cool {
    
    bool gSemantDebug = false;
    
    // Predefined symbols used in Cool
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
    
    
    
    Symbol* InheritanceGraph::LeastUpperBound(Symbol* klass1, Symbol* klass2) {
        	//Symbol* no_t = gIdentTable.lookup(std::string("No_type"));
        	if (klass1 == No_type) {
        		return klass2;
        	} else if (klass2 == No_type) {
        		return klass1;
        	} else {
        		return LeastUpperBound(ClassFind(klass1), ClassFind(klass2))->klass()->name();
        	}
        }
    
    InheritanceGraph::InheritanceGraph(Klasses* klasses, SemantError& error): KlassTable<InheritanceGraphNode>(), error_(error) {
        	root_ = ClassFind(Object);
            InstallClasses(klasses);
            ConnectNodes();
            CheckAcyclic();
        }
        
    InheritanceGraphNode* InheritanceGraph::LeastUpperBound(InheritanceGraphNode* node1, InheritanceGraphNode* node2) {
        	// if either node is invalid class or forms cycle, generate error and return Object
        	if (!acyclic[node1]) {
        		error_(node1->klass()) << "no valid least upper bound for invalid type " << node1->klass()->name() << "." << std::endl;
        		return root_;
        	} else if (!acyclic[node2]) {
        		error_(node2->klass()) << "no valid least upper bound for invalid type " << node2->klass()->name() << "." << std::endl;
        		return root_;
        	}
        
        	std::vector<InheritanceGraphNode*> inheritance1, inheritance2;
        	InheritanceGraphNode* orig = node1;
        	while (true) {
        		inheritance1.push_back(node1);
        		if (node1 == root_)
        			break;
        		node1 = node1->parent_;
        	}
        	while (true) {
        		inheritance2.push_back(node2);
        		if (node2 == root_)
        			break;
        		node2 = node2->parent_;
        	}
        	std::vector<InheritanceGraphNode*>::reverse_iterator ancestor1, ancestor2;
        	for (ancestor1 = inheritance1.rbegin(), ancestor2 = inheritance2.rbegin();
        		ancestor1 != inheritance1.rend() && ancestor2 != inheritance2.rend() && *ancestor1 == *ancestor2;
        		++ancestor1, ++ancestor2) {}
        	
        	if (ancestor1 != inheritance1.rend()) {
        		return *(--ancestor1);
        	} else if (ancestor2 != inheritance2.rend()) {
        		return *(--ancestor2);
        	} else {
        		return orig;
        	}
        }
        
    bool InheritanceGraph::IsClassValid(Symbol* klass_name) {
    		if (klass_name == SELF_TYPE) {
    			return true;
    		}
        	InheritanceGraphNode* klass = ClassFind(klass_name);
        	if (klass == nullptr) {
        		return false;
        	} else {
        		return acyclic[klass];
        	}
        }
        
    
    Symbol* SemantEnv::LeastUpperBound(Symbol* t1, Symbol* t2, Klass* current_klass) {
    		if (t1 == No_type) {
    			return t2;
    		} else if (t2 == No_type) {
    			return t1;
    		} else if (t1 == t2) {
    			// properly handles case where both types are SELF_TYPE
    			return t1;
    		} else {
    			Symbol *t1_noself, *t2_noself;
    			t1_noself = (t1 == SELF_TYPE)? current_klass->name() : t1;
    			t2_noself = (t2 == SELF_TYPE)? current_klass->name() : t2;
    			return C_.LeastUpperBound(t1_noself, t2_noself);	
    		}
    	}

    bool SemantEnv::IsLessOrEqual(Symbol* lesser, Symbol* greater, Klass* current_klass) {
    		if (lesser == No_type || greater == No_type) {
    			return true;
    		} else if (lesser == SELF_TYPE && greater == SELF_TYPE) {
    			return true;
    		} else if (lesser == SELF_TYPE) {
    			return C_.InheritsFrom(current_klass->name(), greater);
    		} else if (greater == SELF_TYPE) {
    			return false;
    		} else {
    			return C_.InheritsFrom(lesser, greater);
    		}
    	}    
    
        
    void SemantEnv::CreateScopedMethodTables() {
    		// start at root node, i.e. Object (since all classes (incl. built-in) inherit from it)
    		ScopedMethodTable empty_table;
    		CreateScopedMethodTablesRec(C_.root(), empty_table);
    		
    		// verify existence of Main class and main() method
    		if (M_.find(Main) == M_.end()) {
    			error_() << "no Main class defined." << std::endl;
    		} else {
    			std::vector<Symbol*>* main_meth_t = M_[Main].Lookup(main_meth);
    			if (main_meth_t == nullptr) {
    				error_(C_.ClassFind(Main)->klass()) << "class Main must define main method." << std::endl;
    			} else if (main_meth_t->size() != 1) {
    				error_(C_.ClassFind(Main)->klass()) << "main method cannot take any formal parameters." << std::endl;
    			}
    		}	
    	}
    
    void SemantEnv::CreateScopedMethodTablesRec(InheritanceGraphNode* node, ScopedMethodTable& inherited_table) {
    		// 0. duplicate inherited table
    		// 1. add methods to new table
    		// 2. recursively create method tables for children
    		Klass* klass = node->klass();
    		Symbol* klass_name = klass->name();
    		ScopedMethodTable& table = M_[klass_name] = inherited_table;	// should create NEW copy of inherited_table
    		table.EnterScope();
    		
    		for (Features::const_iterator feature = klass->features_begin(); feature != klass->features_end(); ++feature) {
    			if ((*feature)->method()) {
    				Method* method = (Method*) *feature;
    				if (table.Probe(method->name())) {
    					error_(klass, method) << "method " << method->name() << " already defined at this scope." << std::endl;
    				} else if (table.Lookup(method->name()) != nullptr) {
    					// check method override signature against inherited method signature
    					std::vector<Symbol*>* method_t = table.Lookup(method->name());
    					
    					if (method->formals()->size() != method_t->size()-1) {
    						error_(klass, method) << "override of inherited method " << method->name() << " must take same number of formals." << std::endl;
    					} else {
    						Formals::const_iterator formal_new = method->formals_begin();
    						std::vector<Symbol*>::const_iterator formal_t = method_t->begin();
    						while (formal_new != method->formals_end()) {
    							if ((*formal_new)->decl_type() != *formal_t) {
    								error_(klass, *formal_new) << "formal type mismatch in override of inherited method " << method->name() << ": expected " << *formal_t << ", found " << (*formal_new)->decl_type() << "." << std::endl;
    								break;
    							}
    							++formal_new, ++formal_t;
    						}
    					}
    					
    					Symbol* ret_t = method_t->back();
    					Symbol* ret_t_new = method->decl_type();
    					if (ret_t != ret_t_new) {
    						error_(klass, method) << "overriding method must have inherited method return type " << ret_t << ", not " << ret_t_new << "." << std::endl;
    					}
    				} else {
    					std::vector<Symbol*>* method_t = new std::vector<Symbol*>();
    					for (Formal* formal : *(method->formals())) {
    						Symbol* formal_t;
    						if (!C_.ClassFind(klass->name())->basic() && !C_.IsClassValid(formal->decl_type())) {
    							error_(klass, formal) << "formal " << formal->name() << " is of undefined type " << formal->decl_type() << "." << std::endl;
    							formal_t = No_type;
    						} else if (formal->decl_type() == SELF_TYPE) {
    							error_(klass, formal) << "formals cannot be of type SELF_TYPE." << std::endl;
    							formal_t = klass->name();
    						} else {
    							formal_t = formal->decl_type();
    						}
    						method_t->push_back(formal_t);
    					}
    					if (!C_.ClassFind(klass->name())->basic() && !C_.IsClassValid(method->decl_type())) {
    						error_(klass , method) << "method " << method->name() << " has invalid return type " << method->decl_type() << "." << std::endl;
    						method_t->push_back(No_type);
    					} else {
    						method_t->push_back(method->decl_type());
    					}
    				
    					table.AddToScope(method->name(), method_t);
    				}
    			}
    		}
    		
    		for (InheritanceGraphNode::child_iterator child = node->children_begin(); child != node->children_end(); ++child) {
    			CreateScopedMethodTablesRec(*child, table);
    		}
    	}
    	
    void SemantEnv::TypeCheckClass(InheritanceGraphNode* node) {
    		Klass* klass = node->klass();
    		Features::const_iterator feature;
    		
    		// Pass 1: add attributes into O
    		O_.EnterScope(); 	// new scope for attributes
    		// first bind self to SELF_TYPE
    		O_.AddToScope(self, SELF_TYPE);
    		for (feature = klass->features_begin(); feature != klass->features_end(); ++feature) {
    			if ((*feature)->attr()) {
    				Attr* attr = (Attr*) *feature;
    				if (O_.Probe(attr->name())) {
    					error_(klass, attr) << "attribute " << attr->name() << " has already been defined in current class." << std::endl;
    				} else if (O_.Lookup(attr->name())) {
    					error_(klass, attr) << "attribute " << attr->name() << " has already been defined in parent class." << std::endl;
    				} else {
    					Symbol* decl_type_t;
    					if (!node->basic() && !C_.IsClassValid(attr->decl_type())) {
    						error_(klass, attr) << "attribute " << attr->name() << " has undefined type " << attr->decl_type() << "." << std::endl;
    						decl_type_t = No_type;
    					} else {
    						decl_type_t = attr->decl_type();
    					}
    					O_.AddToScope(attr->name(), decl_type_t);
    				}
    			}
    		}
    		//std::cerr << "Typechecking class " << klass->name() << std::endl;
    		for (feature = klass->features_begin(); feature != klass->features_end(); ++feature) {
    			(*feature)->TypeCheck(C_, *this, klass);
    		}
    		    		
    		// call child classes recursively
    		for (InheritanceGraphNode::child_iterator child = node->children_begin(); child != node->children_end(); ++child) {
    			TypeCheckClass(*child);
    		}
    		
    		O_.ExitScope();
    	}
    
    
    
    void BoolLiteral::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	set_type(Bool);
    }
    
    void IntLiteral::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	set_type(Int);
    }
    
    void StringLiteral::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	set_type(gIdentTable.lookup(std::string("String")));
    }
    
    void NoExpr::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	set_type(No_type);
    }
    
    void Ref::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {       
    	Symbol* t = env.LookupO(name_);
    	if (t == nullptr) {
    		env.error()(klass, this) << "undeclared identifier " << name_ << "." << std::endl;
    		// error recovery: set to root of inheritance tree, i.e. Object
    		set_type(g.root()->klass()->name());
    	} else {
    		set_type(t);
    	}
    }
    
    void BinaryOperator::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	Symbol *lhst, *rhst;
    	lhs_->TypeCheck(g, env, klass);
    	rhs_->TypeCheck(g, env, klass);
    	lhst = lhs_->type();
    	rhst = rhs_->type();
    	switch (kind_) {
    	case BO_Add:
    	case BO_Sub:
    	case BO_Mul:
    	case BO_Div:
    	case BO_LT:
    	case BO_LE:
    		// lhs & rhs must be of type Int
    		if (lhst != rhst || lhst != Int) {
    			// error
    			env.error()(klass, this) << "binary operator " << KindAsString() << " requires types Int and Int, not " << lhst << " and " << rhst << "." << std::endl;
    		}
    		if (kind_ == BO_LT || kind_ == BO_LE) {
    			set_type(Bool);
    		} else {
    			set_type(Int);
    		}
    		break;
    	case BO_EQ:
    		std::set<Symbol*> special_t;
    		special_t.insert(Bool);
    		special_t.insert(Int);
    		special_t.insert(String);
    		if ((special_t.find(lhst) != special_t.end() || special_t.find(rhst) != special_t.end()) && lhst != rhst) {
    			// error: type mismatch
    			env.error()(klass, this) << "equality operator incompatible with types " << lhst << " and " << rhst << "." << std::endl;
    		}
    		set_type(Bool);
    		break;
    	}
    }
    
    void UnaryOperator::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	input_->TypeCheck(g, env, klass);
    	Symbol* t = input_->type();
    	switch(kind_) {
    	case UO_Neg:
    		if (t != gIdentTable.lookup(std::string("Int"))) {
    			// error
    			env.error()(klass, this) << "negation requires operand of type Int, not " << t << "." << std::endl;
    		}
    		set_type(gIdentTable.lookup(std::string("Int")));
    		break;
    	case UO_Not:
    		if (t != gIdentTable.lookup(std::string("Bool"))) {
    			// error
    			env.error()(klass, this) << "'not' requires operand of type Bool, not " << t << "." << std::endl;
    		}
    		set_type(gIdentTable.lookup(std::string("Bool")));
    		break;
    	case UO_IsVoid:
    		set_type(gIdentTable.lookup(std::string("Bool")));
    		break;
    	}
    }
    
    void Knew::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	if (!g.IsClassValid(name_)) {
    		env.error()(klass, this) << "encountered undefined type " << name_ << " in 'new' expression." << std::endl;
    		set_type(No_type);
    	} else if (name_ == SELF_TYPE) {
    		// handle SELF_TYPE case explicitly for clarity, even though same code
    		set_type(name_);
    	} else {
    		// apply standard type rules
    		set_type(name_);
    	}
    }
    
    void KaseBranch::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	Symbol* decl_type_t;
    	if (!g.IsClassValid(decl_type_)) {
    		env.error()(klass, this) << "identifier " << name_ << " has undefined type " << decl_type_ << "." << std::endl;
    		decl_type_t = No_type;
    	} else if (decl_type_ == SELF_TYPE) {
    		env.error()(klass, this) << "identifier in case branch cannot be of SELF_TYPE." << std::endl;
    		// error recovery: set current type to klass
    		decl_type_t = klass->name();
    	} else {
    		decl_type_t = decl_type_;
    	}
    	env.O().EnterScope();
    	env.O().AddToScope(name_, decl_type_t);
    	body_->TypeCheck(g, env, klass);
    	env.O().ExitScope();
    	
    	set_type(body_->type());    	
    }
    
    void Kase::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	input_->TypeCheck(g, env, klass);
    	Symbol* union_t = No_type;
    	std::set<Symbol*> branch_types;
    	for (KaseBranch* branch : *cases_) {
    		branch->TypeCheck(g, env, klass);
			union_t = env.LeastUpperBound(union_t, branch->type(), klass);
    		if (branch_types.find(branch->decl_type()) != branch_types.end()) {
    			// branches must have ID's of distinct types
    			env.error()(klass, this) << "the variables declared in each case branch must have distinct types." << std::endl;
    		} else {
    			branch_types.insert(branch->decl_type());
    		}
    	}
    	set_type(union_t);
    }
    
    void Let::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	Symbol *id_t, *init_t, *body_t, *decl_type_t;
    	init_->TypeCheck(g, env, klass);
    	init_t = init_->type();
    	if (!g.IsClassValid(decl_type_)) {
    		env.error()(klass, this) << "declared type " << decl_type_ << " of identifier " << name_ << " is undefined." << std::endl;
    		decl_type_t = No_type;
    	} else {
    		decl_type_t = decl_type_;
    	}
    	if (!env.IsLessOrEqual(init_t, decl_type_t, klass)) {
    		// error: init_expr not <= decl_type
    		env.error()(klass, this) << "type of initializer (" << init_t << ") is not less than or equal to identifier's declared type (" << decl_type_t << ")." << std::endl;
    		// error recovery: ID now is of "greatest" type, i.e. init_t
    		id_t = init_t;
    	} else {
    		id_t = decl_type_t;
    	}
    	
    	env.O().EnterScope();
    	env.O().AddToScope(name_, id_t);
    	body_->TypeCheck(g, env, klass);
    	env.O().ExitScope();
    	
    	body_t = body_->type();
    	set_type(body_t);
    }
    
    void Block::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	//expressions* body_
    	for (Expression* expr : *body_) {
    		expr->TypeCheck(g, env, klass);
    	}
    	set_type(body_->back()->type());
    }
    
    void Loop::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	Symbol *predt, *bodyt;
    	pred_->TypeCheck(g, env, klass);
    	body_->TypeCheck(g, env, klass);
    	predt = pred_->type();
    	bodyt = body_->type();
    	if (predt != gIdentTable.lookup(std::string("Bool"))) {
    		// error: predicate must be of type bool
    		env.error()(klass, this) << "predicate must be of type Bool, not " << predt << "." << std::endl;
    	}
    	set_type(gIdentTable.lookup(std::string("Object")));
    }
    
    void Cond::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	Symbol *predt, *thent, *elset;
    	pred_->TypeCheck(g, env, klass);
    	then_branch_->TypeCheck(g, env, klass);
    	else_branch_->TypeCheck(g, env, klass);
    	predt = pred_->type();
    	thent = then_branch_->type();
    	elset = else_branch_->type();
    	if (predt != gIdentTable.lookup(std::string("Bool"))) {
    		env.error()(klass, this) << "predicate must be of type Bool, not " << predt << "." << std::endl;
    	}
    	set_type(env.LeastUpperBound(thent, elset, klass));
    }
    
    void StaticDispatch::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	Symbol *dispatch_t, *receiver_t;
    	if (dispatch_type_ == SELF_TYPE) {
    		// error: dispatch type cannot be SELF_TYPE
    		env.error()(klass, this) << "dispatch type cannot be SELF_TYPE." << std::endl;
    		dispatch_t = klass->name();
    	} else {
    		dispatch_t = dispatch_type_;
    	}
    	
    	receiver_->TypeCheck(g, env, klass);
    	receiver_t = receiver_->type();
    	
    	std::vector<Symbol*> actual_ts;
    	for (Expression* expr : *actuals_) {
    		expr->TypeCheck(g, env, klass);
    		actual_ts.push_back(expr->type());
    	}
    	
    	if (!g.IsClassValid(dispatch_t)) {
    		env.error()(klass, this) << "undefined dispatch type " << dispatch_t << "." << std::endl;
    		set_type(Object);
    		return;
    	}
    	
    	std::vector<Symbol*>* method_t = env.M(g.ClassFind(dispatch_t)->klass()).Lookup(name_);
    	if (method_t == nullptr) {
    		env.error()(klass, this) << "class " << dispatch_t << " has no method " << name_ << "." << std::endl;
    		set_type(Object);
    	} else if (!env.IsLessOrEqual(receiver_t, dispatch_t, klass)) {
    		env.error()(klass, this) << "cannot perform static dispatch on object of type " << receiver_t << " that does not inherit from type " << dispatch_t << "." << std::endl;
    		set_type(Object);
    	} else {
    		if (actual_ts.size() != method_t->size()-1) {
    			env.error()(klass, this) << "method requires " << method_t->size()-1 << " parameters, but got " << actual_ts.size() << "." << std::endl;
    		} else {
    			std::vector<Symbol*>::iterator actual_t = actual_ts.begin();
    			auto formal = method_t->begin();
    			int i = 1;
    			while (actual_t != actual_ts.end()) {
    				if (!env.IsLessOrEqual(*actual_t, *formal, klass)) {
    					env.error()(klass, this) << "for parameter " << i << " of method, expected type " << *formal << " but encountered type " << *actual_t << "." << std::endl;
    				}
    				++i, ++actual_t, ++formal;
    			}
    		}
    		if (method_t->back() == SELF_TYPE) {
    			// handle case of SELF_TYPE return type explicitly for clarity
    			set_type(receiver_t);
    		} else {
    			set_type(method_t->back());
    		}
    	}
    }
    
    void Dispatch::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {    	
    	receiver_->TypeCheck(g, env, klass);
    	Symbol* receiver_t;
    	if (receiver_->type() == SELF_TYPE) {
    		receiver_t = klass->name();
    	} else {
    		receiver_t = receiver_->type();
    	}
    	
    	std::vector<Symbol*> actual_ts;
    	for (Expression* expr : *actuals_) {
    		expr->TypeCheck(g, env, klass);
    		actual_ts.push_back(expr->type());
    	}
    	
    	std::vector<Symbol*>* method_t = env.M(g.ClassFind(receiver_t)->klass()).Lookup(name_);
    	if (method_t == nullptr) {
    		env.error()(klass, this) << "class " << receiver_t << " has no method " << name_ << "." << std::endl;
    		set_type(Object);
    	} else {
    		if (actual_ts.size() != method_t->size()-1) {
    			env.error()(klass, this) << "method requires " << method_t->size()-1 << " parameters, but got " << actual_ts.size() << "." << std::endl;
    		} else {
    			std::vector<Symbol*>::iterator actual_t = actual_ts.begin();
    			auto formal = method_t->begin();
    			int i = 1;
    			while (actual_t != actual_ts.end()) {
    				if (!env.IsLessOrEqual(*actual_t, *formal, klass)) {
    					env.error()(klass, this) << "for parameter " << i << " of method, expected type " << *formal << " but encountered type " << *actual_t << "." << std::endl;
    				}
    				++i, ++actual_t, ++formal;
    			}
    		}
    		if (method_t->back() == SELF_TYPE) {
    			// explicitly handle case when return type is SELF_TYPE for clarity
    			set_type(receiver_->type());
    		} else {
    			set_type(method_t->back());
    		}
    	}
    	
    }
    
    void Assign::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	Symbol *obj_t, *val_t;    	
    	value_->TypeCheck(g, env, klass);
    	val_t = value_->type();
    	obj_t = env.O().Lookup(name_);
    	if (obj_t == nullptr) {
    		env.error()(klass, this) << "object identifier " << name_ << " not declared in this scope." << std::endl;
    		// error recovery: set type to val_t
    		obj_t = val_t;
    	}
    	if (!env.IsLessOrEqual(val_t, obj_t, klass)) {
    		// error: must be case that val_t ≤ obj_t
    		env.error()(klass, this) << "assignment value of type " << val_t << " must be less than or equal to object's type " << obj_t << "." << std::endl;
    	}
    	set_type(val_t);
    }
    
    void Attr::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {
    	Symbol *init_t, *decl_type_t;
    	init_->TypeCheck(g, env, klass);
    	init_t = init_->type();
    	
    	if (!g.ClassFind(klass->name())->basic() && !g.IsClassValid(decl_type_)) {
    		env.error()(klass, this) << "attribute " << name_ << " has undefined type " << decl_type_ << std::endl;
    		decl_type_t = No_type;
    	} else {
    		decl_type_t = decl_type_;
    	}
    	
    	if (!env.IsLessOrEqual(init_t, decl_type_t, klass)) {
    		env.error()(klass, this) << "attribute initializer cannot be of type " << init_t << " that is less than or equal to attribute's declared type " << decl_type_ << std::endl;
    	}
    }
    
    void Method::TypeCheck(InheritanceGraph& g, SemantEnv& env, Klass* klass) {    
    	// add formals to O
    	env.O().EnterScope();
    	    	
    	std::vector<Symbol*>::iterator formal_t = env.M(klass).Lookup(name())->begin();
    	Formals::const_iterator formal = formals_begin();
    	while (formal != formals_end()) {
    		env.O().AddToScope((*formal)->name(), *formal_t);
    		++formal, ++formal_t;
    	}
    	body_->TypeCheck(g, env, klass);

    	if (!env.IsLessOrEqual(body_->type(), *formal_t, klass)) {
    		env.error()(klass, this) << "method body returns type " << body_->type() << " that is not less than or equal to declared return type " << *formal_t << "." << std::endl;
    	}
    	
    	env.O().ExitScope();
    }
        
    void Semant(Program* program) {
        // Initialize error tracker (and reporter)
        SemantError error(std::cerr);
        
        // Initialize Cool "built-in" symbols
        InitCoolSymbols();
        
        // PASS 1
    	// constructing class table automatically checks for redefinition and
    	// validity of inheritance graph
        InheritanceGraph inheritanceGraph(program->klasses(), error);
        
        // PASS 2
        // Construct O,M,C environment
        // C: class declarations
        // M: method definitions
        // O: class attributes, formal parameters, etc
        // NOTE: attributes are local to a class
        // *** recursive approach to M: only need to use one scoped table, just start
        //		at root of inheritance graph, then recursively eval child classses
        SemantEnv semantEnv(inheritanceGraph, error);
        semantEnv.TypeCheck();
        
        // Halt program with non-zero exit if there are semantic errors
        if (error.errors()) {
            std::cerr << "Compilation halted due to static semantic errors." << std::endl;
            exit(1);
        }

    }
    
} // namespace cool

