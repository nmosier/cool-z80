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

#pragma once

#include <assert.h>

#include "ast.h"
#include "ast_consumer.h"
#include "stringtab.h"
#include "scopedtab.h"

namespace cool {
    
    /**
     * Set to true to display debig output
     */
    extern bool gSemantDebug;
    
    /**
     * Main entry point for semantic analysis
     * @param program
     */
    void Semant(Program* program);
    
    /**
     * Provide error tracking and structured printing of semantic errors
     */
    class SemantError {
    public:
        SemantError(std::ostream& os) : os_(os), errors_(0) {}
        
        std::size_t errors() const { return errors_; }
        
        /**
         * @}
         * @name Error reporting
         * @{
         */
        std::ostream& operator()(const Klass* klass) { return (*this)(klass->filename(), klass); }
        std::ostream& operator()(const Klass* klass, const ASTNode* node) {
            return (*this)(klass->filename(), node);
        }
        
        std::ostream& operator()(const StringLiteral* filename, const ASTNode* node) {
            errors_++;
            os_ << filename << ":" << node->loc() << ": ";
            return os_;
        }
        
        std::ostream& operator()() {
            errors_++;
            return os_;
        }
        
    private:
        std::ostream& os_;
        std::size_t errors_;
    };
    
    class InheritanceGraph;
    
    class InheritanceGraphNode : public InheritanceNode<InheritanceGraphNode> {
    public:    	    	
    	InheritanceGraphNode(Klass* klass, bool inheritable, bool basic): InheritanceNode<InheritanceGraphNode>(klass, inheritable, basic) {}
    	
     	typedef std::vector<InheritanceGraphNode*>::const_iterator child_iterator;
		child_iterator children_begin() { return children_.begin(); }
		child_iterator children_end() { return children_.end(); }
	
    protected:    
    	friend class InheritanceGraph;
    };
    
    class InheritanceGraph : public KlassTable<InheritanceGraphNode> {
    public:
        InheritanceGraph(Klasses* klasses, SemantError& error);
        
        InheritanceGraphNode* LeastUpperBound(InheritanceGraphNode* node1, InheritanceGraphNode* node2);
        Symbol* LeastUpperBound(Symbol* klass1, Symbol* klass2);
        
        bool InheritsFrom(InheritanceGraphNode* child, InheritanceGraphNode* parent) {
        	return LeastUpperBound(child, parent) == parent;
        }
        bool InheritsFrom(Symbol* child_t, Symbol* parent_t) {
        	return LeastUpperBound(child_t, parent_t) == parent_t;
        }
        
        bool IsClassValid(Symbol* klass_name);
        
        // prints out inheritance graph to std::cout
        void TraverseTree() {
        	TraverseTree_Rec(root_, 0);
        }
        
    protected:
    	SemantError& error_;
    	std::unordered_map<InheritanceGraphNode*,bool> acyclic;
    	
    	void TraverseTree_Rec(InheritanceGraphNode* root, int level) {
    		std::cout << std::string(level, '\t') << root->name() << std::endl;
    		for (InheritanceGraphNode* child : root->children_) {
    			TraverseTree_Rec(child, level+1);
    		}
    	}  
    	  
        void InstallClasses(Klasses *klasses) {
            for (Klass *klass : *klasses) {
                if (ClassFind(klass->name())) {
                    // Class redefinition error
                    error_(klass) << "class " << klass->name() << " already defined." << std::endl;
                } else {
                	InstallClass(new InheritanceGraphNode(klass, true, false));                    
                }
            }
        }
        
        void ConnectNodes() {
        	for (InheritanceGraphNode* isolated_node : nodes_) {
        		InheritanceGraphNode* parent_node;
        		if ((parent_node = ClassFind(isolated_node->parent_name()))) {
        			if (parent_node->inheritable()) {
        				parent_node->children_.push_back(isolated_node);
        				isolated_node->parent_ = parent_node;
        			} else {
        				error_(isolated_node->klass()) << "class " << isolated_node->klass()->name() << " cannot inherit from class " << isolated_node->parent_name() << "." << std::endl;
        			}
        		} else {
					error_(isolated_node->klass()) << "class " << isolated_node->klass()->name() <<" cannot inherit from undefined class " << isolated_node->parent_name() << std::endl;
        		}
        	}
        }
        
        void CheckAcyclic() {
        	MarkAcyclicRec(root_);
        	for (InheritanceGraphNode* node : nodes_) {
        		if (acyclic.find(node) == acyclic.end()) {
        			// know that class forms cycle, since doesn't inherit from Object
        			InheritanceGraphNode* ancestor;
        			acyclic[node] = false;
        			for (ancestor = node; ancestor->parent_ && ancestor->parent_ != node; ancestor = ancestor->parent_) {
        				acyclic[ancestor] = false;
        			}
        			acyclic[ancestor] = false;
        			if (ancestor->parent_ == nullptr) {
        				if (node != ancestor) {
        					error_(node->klass()) << "class " << node->klass()->name() << " inherits from invalid class " << node->parent_->klass()->name() << std::endl;
        				}
        			} else if (node->parent_ == node) {
        				error_(node->klass()) << "class " << node->klass()->name() << " cannot inherit from itself." << std::endl;
        			} else {
        				for (ancestor = node; ancestor->parent_ && ancestor->parent_ != node; ancestor = ancestor->parent_) {
        					error_(node->klass()) << "class " << ancestor->klass()->name() << " forms cycle with class " << ancestor->parent_->klass()->name() << "." << std::endl;
        				}
        				error_(node->klass()) << "class " << ancestor->klass()->name() << " forms cycle with class " << node->klass()->name() << "." << std::endl;
        			}
        		}
        	}
        }
        
        void MarkAcyclicRec(InheritanceGraphNode* acyclic_node) {
        	acyclic[acyclic_node] = true;
        	for (InheritanceGraphNode* acyclic_child : acyclic_node->children_) {
        		MarkAcyclicRec(acyclic_child);
        	}
        }
    };
    
    
    class SemantEnv {
    public:
    	typedef ScopedTable<Symbol*, std::vector<Symbol*> > ScopedMethodTable;
    	typedef std::unordered_map<Symbol*, ScopedMethodTable> ScopedMethodTables;
    	typedef ScopedTable<Symbol*, Symbol> ScopedObjectTable;
    
    	SemantEnv(InheritanceGraph& C, SemantError& error): C_(C), error_(error) {}
    	
    	SemantError& error() { return error_; }
    	
    	// perform type checks
    	void TypeCheck() {
    		// 1. start at root of inheritance graph
    		// 2. initialize/update M_ and O_ tables with methods & attributes of class, respectively
    		// 3. typecheck class attribute initializations
    		// 4. typecheck method bodies
    		// 5. recursively typecheck children
    		
    		CreateScopedMethodTables();
    		TypeCheckClass(C_.root());
    	}
    	
    	Symbol* LookupO(Symbol* id) {
    		return O_.Lookup(id);
    	}
    	
    	ScopedObjectTable& O() {
    		return O_;
    	}
    	
    	ScopedMethodTable M(Klass* klass) {
    		return M_[klass->name()];
    	}
    	
    	// extended version of LUB function
    	// to handle SELF_TYPE
    	Symbol* LeastUpperBound(Symbol* t1, Symbol* t2, Klass* current_klass);
    	
    	// extended version of ≤ function
    	// to handle SELF_TYPE
    	bool IsLessOrEqual(Symbol* lesser, Symbol* greater, Klass* current_klass);
    	
    protected:    	
    	InheritanceGraph& C_;
    	ScopedMethodTables M_;
    	//ScopedMethodTable M_;	// delete later
    	ScopedObjectTable O_;
    	
    	SemantError& error_;
    	
    	void CreateScopedMethodTables();
    	void CreateScopedMethodTablesRec(InheritanceGraphNode* node, ScopedMethodTable& inherited_table);
    	
    	void TypeCheckClass(InheritanceGraphNode* node);
    	
    };
    
}



