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

#include <unordered_map>
#include <vector>
#include <cassert>

#include "ast.h"

namespace cool {

template<class Node>
class KlassTable;

/**
 * Base class for nodes in the inheritance graph
 * @tparam Node
 */
template<class Node>
class InheritanceNode {
 public:
  /**
   * @param klass Klass ASTNode for this inheritance graph node
   * @param inheritable true if class can be inherited from (true for all user-defined classes)
   * @param basic true if basic class, e.g. Bool (false for all user-defined classes)
   * @return
   */
  InheritanceNode(Klass* klass, bool inheritable, bool basic) :
      klass_(klass), inheritable_(inheritable), basic_(basic) {}
  virtual ~InheritanceNode() = default;

  /**
   * @return Klass ASTNode for this inheritance graph node
   */
  Klass* klass() const { return klass_; }

  /**
   * @return name of the associated Klass
   */
  Symbol* name() const { return klass_->name(); }
  StringLiteral* filename() const { return klass_->filename(); }

  /**
   * @return Parent node
   */
  Node* parent() const { return parent_; }

  /**
   * @return Name of the parent class
   */
  Symbol* parent_name() const { return klass_->parent(); }

  /**
   * Can classes inherit from this class?
   * @return
   */
  bool inheritable() const { return inheritable_; }

  /**
   * Is this a basic (built-in) class?
   * @return
   */
  bool basic() const { return basic_; }

 protected:
  Klass* klass_;
  bool inheritable_;
  bool basic_;

  /**
   * Parent node in the inheritance graph
   */
  Node* parent_ = nullptr;

  /**
   * Child nodes of this class
   */
  std::vector<Node*> children_;

  friend class KlassTable<Node>;
};

template<class Node>
class KlassTable {
 public:
  KlassTable() {
    InstallBasicClasses();
    // Sub-class constructors are responsible for building inheritance graph
  }

  virtual ~KlassTable() {
    // Delete all nodes in table (includes special classes and nodes in the inheritance graph)
    for (auto & entry : node_table_) {
      delete entry.second;
    }
  }

  /**
   * Get the root of the inheritance graph.
   *
   * By the Cool specification, the root of the inheritance graph must always be Object, thus this root node
   * is automatically set to the Node for the Object class.
   *
   * @return The root of the inheritance graph (the Object class)
   */
  Node* root() const { return root_; }

  /**
   * Find the inheritance node for a class by name
   * @param name Class name
   * @return InheritanceNode or nullptr if class not found
   */
  Node* ClassFind(Symbol* name) const {
    auto found_node = node_table_.find(name);
    return (found_node != node_table_.end()) ? found_node->second : nullptr;
  }

 protected:
  /**
   * Root of the inheritance tree
   */
  Node* root_ = nullptr;

  /**
   * Map classes names to InheritanceNodes. Includes "special" classes
   * used by the compiler internally.
   */
  std::unordered_map<Symbol*, Node*> node_table_;

  /**
   * Track InheritanceNodes for only classes that can appear in the inheritance graph.
   * Excludes "special" classes used by the compiler internally.
   */
  std::vector<Node*> nodes_;

  /**
   * Add node to name-node map, but not container tracking nodes in the inheritance graph.
   * @param node
   */
  void AddNode(Node* node) {
    node_table_.emplace(node->name(), node);
  }

  /**
   * Install Cool built-in classes into class table
   *
   * Alongside installing basic classes, this methods sets the root
   * of the tree to Object class.
   */
  void InstallBasicClasses();

  /**
   * Install node in inheritance graph into class table
   * @param node
   */
  void InstallClass(Node *node) {
    auto name = node->name();
    auto added_node = node_table_.emplace(name, node);
    assert(added_node.second); // Should have already checked for unique class name
    nodes_.push_back(node);
  }

  /**
   * Install user-defined Cool classes into class table
   * @param klasses
   */
  void InstallClasses(Klasses *klasses) {
    for (auto klass : *klasses)
      InstallClass(new Node(klass, true /*CanInherit*/, false /*NotBasic*/));
  }

};

/**
 * Emplace built-in Cool symbols in gIdentTable. You must invoked this function
 * before using any of the pre-defined symbols. 
 */
void InitCoolSymbols();

/**
 * @}
 * @name Create instances of built-in classes for use in AST analysis passes
 * {@
 */
Klass* CreateNoClassKlass();
Klass* CreateSELF_TYPEKlass();
Klass* CreatePrimSlotKlass();
Klass* CreateObjectKlass();
Klass* CreateIOKlass();
Klass* CreateIntKlass();
Klass* CreateBoolKlass();
Klass* CreateStringKlass();

template<class Node>
void KlassTable<Node>::InstallBasicClasses() {

  // Special classes that are installed in the class table but are not part of the inheritance graph:
  //  No_class serves as the parent of Object and the other special classes.
  //  SELF_TYPE is the self class; it cannot be redefined or inherited.
  //  prim_slot is a class known to the code generator.
  AddNode(new Node(CreateNoClassKlass(), true /* CanInherit */, true /* Basic */));
  AddNode(new Node(CreateSELF_TYPEKlass(), false /* CantInherit */, true /* Basic */));
  AddNode(new Node(CreatePrimSlotKlass(), false/* CantInherit */, true /* Basic */));

  // Basic classes installed in both the class table and inheritance graph
  InstallClass(root_ = new Node(CreateObjectKlass(), true /*CanInherit*/, true /*Basic*/));
  InstallClass(new Node(CreateIOKlass(), true /*CanInherit*/, true /*Basic*/));
  InstallClass(new Node(CreateIntKlass(), false /*CantInherit*/, true /*Basic*/));
  InstallClass(new Node(CreateBoolKlass(), false /*CantInherit*/, true /*Basic*/));
  InstallClass(new Node(CreateStringKlass(), false /*CantInherit*/, true /*Basic*/));
}



}
