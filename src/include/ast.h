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

#include <string>
#include <iosfwd>

#include "stringtab.h"
#include "utilities.h"
#include "ast_fwd.h"

namespace cool {

// Forward declare semantic analysis and code generation environments

/// Abstract base class for all AST Nodes
class ASTNode {
 public:
  const SourceLoc& loc() const { return loc_; }


  /// Dump AST as a formatted tree readable by Cool compiler phases
  /// \param os Output stream
  /// \param level Indentation level
  /// \param with_types Include Expression types
  virtual void DumpTree(std::ostream& os, size_t level, bool with_types) const = 0;

 protected:
  SourceLoc loc_ = 0;

  ASTNode() {}
  ASTNode(SourceLoc loc) : loc_(loc) {}

  void DumpLine(std::ostream& os, size_t level) const {
    pad(os, level) << '#' << loc_ << std::endl;
  }
};

/// Cool program
class Program : public ASTNode {
 public:
  // All AST nodes are created with factory methods
  static Program* Create(Klasses* klasses, SourceLoc loc = 0);

  Klasses* klasses() { return klasses_; }

  // C++11 Note: override specifier ensures we are actually overriding a virtual function
  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Klasses* klasses_;

  Program(Klasses* klasses, SourceLoc loc) : ASTNode(loc), klasses_(klasses) {}
};

/// Cool class
class Klass : public ASTNode {
 public:
  static Klass* Create(Symbol* name, Symbol* parent, Features* features, StringLiteral* filename,
                       SourceLoc loc = 0);

  Symbol* name() const { return name_; }
  Symbol* parent() const { return parent_; }
  StringLiteral* filename() const { return filename_; }

  const Features* features() const { return features_; }

  Features::size_type features_size() const { return features_->size(); }
  Feature* feature(size_t i) const { return features_->at(i); }
  Features::const_iterator features_begin() const { return features_->begin(); }
  Features::const_iterator features_end() const { return features_->end(); }

  Attr* attr(Symbol* name) const;
  Method* method(Symbol* name) const;

  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Symbol* name_;
  Symbol* parent_;
  Features* features_;
  StringLiteral* filename_;

  Klass(Symbol* name, Symbol* parent, Features* features, StringLiteral* filename, SourceLoc loc)
      : ASTNode(loc), name_(name), parent_(parent), features_(features), filename_(filename) {}
};

/// Cool formal argument
class Formal : public ASTNode {
 public:
  static Formal* Create(Symbol* name, Symbol* decl_type, SourceLoc loc = 0);

  Symbol* name() const { return name_; }
  Symbol* decl_type() const { return decl_type_; }


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Symbol* name_;
  Symbol* decl_type_;

  Formal(Symbol* name, Symbol* decl_type, SourceLoc loc)
      : ASTNode(loc), name_(name), decl_type_(decl_type) {}
};

/// Abstract base class for Cool Class features
class Feature : public ASTNode {
 public:
  virtual bool method() const { return false; }
  virtual bool attr() const { return false; }

  Symbol* name() const { return name_; }
  Symbol* decl_type() const { return decl_type_; }


 protected:
  Symbol* name_;
  Symbol* decl_type_;

  Feature(Symbol* name, Symbol* decl_type, SourceLoc loc)
      : ASTNode(loc), name_(name), decl_type_(decl_type) {}
};

/// Cool method definition
class Method : public Feature {
 public:
  static Method* Create(Symbol* name, Formals*, Symbol* decl_type, Expression* body,
                        SourceLoc loc = 0);

  bool method() const override { return true; }

  const Formals* formals() const { return formals_; }

  Formals::size_type formals_size() const { return formals_->size(); }
  Formal* formal(std::size_t i) const { return formals_->at(i); }
  Formals::const_iterator formals_begin() const { return formals_->begin(); }
  Formals::const_iterator formals_end() const { return formals_->end(); }


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Formals* formals_;
  Expression* body_;

  Method(Symbol* name, Formals* formals, Symbol* decl_type, Expression* body, SourceLoc loc)
      : Feature(name, decl_type, loc), formals_(formals), body_(body) {}
};

/// Cool attribute definition
class Attr : public Feature {
 public:
  static Attr* Create(Symbol* name, Symbol* decl_type, Expression* init, SourceLoc loc = 0);

  bool attr() const override { return true; }

  Expression* init() const { return init_; }


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Expression* init_;

  Attr(Symbol* name, Symbol* decl_type, Expression* init, SourceLoc loc)
      : Feature(name, decl_type, loc), init_(init) {}
};

/// Base class for typed expression
class Expression : public ASTNode {
 public:
  Symbol* type() const { return type_; }
  void set_type(Symbol* type) { type_ = type; }


  void DumpType(std::ostream& os, size_t level, bool with_types) const;

 protected:
  Symbol* type_;

  Expression(SourceLoc loc) : ASTNode(loc), type_(nullptr) {}
};

/// Cool assign expression
class Assign : public Expression {
 public:
  static Assign* Create(Symbol* name, Expression* value, SourceLoc loc = 0);


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Symbol* name_;
  Expression* value_;

  Assign(Symbol* name, Expression* value, SourceLoc loc)
      : Expression(loc), name_(name), value_(value) {}
};

/// Cool method dispatch
class Dispatch : public Expression {
 public:
  static Dispatch* Create(Expression* receiver, Symbol* name, Expressions* actuals,
                          SourceLoc loc = 0);


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Expression* receiver_;
  Symbol* name_;
  Expressions* actuals_;

  Dispatch(Expression* receiver, Symbol* name, Expressions* actuals, SourceLoc loc)
      : Expression(loc), receiver_(receiver), name_(name), actuals_(actuals) {}
};

/// Cool static method dispatch
class StaticDispatch : public Dispatch {
 public:
  static StaticDispatch* Create(Expression* receiver, Symbol* dispatch_type, Symbol* name,
                                Expressions* actuals, SourceLoc loc = 0);


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Symbol* dispatch_type_;

  StaticDispatch(Expression* receiver, Symbol* dispatch_type, Symbol* name, Expressions* actuals,
                 SourceLoc loc)
      : Dispatch(receiver, name, actuals, loc), dispatch_type_(dispatch_type) {}
};

/// Cool conditional (i.e. if-then-else)
class Cond : public Expression {
 public:
  static Cond* Create(Expression* pred, Expression* then_branch, Expression* else_branch,
                      SourceLoc loc = 0);


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Expression* pred_;
  Expression* then_branch_;
  Expression* else_branch_;

  Cond(Expression* pred, Expression* then_branch, Expression* else_branch, SourceLoc loc)
      : Expression(loc), pred_(pred), then_branch_(then_branch), else_branch_(else_branch) {}
};

/// Cool while loop
class Loop : public Expression {
 public:
  static Loop* Create(Expression* pred, Expression* body, SourceLoc loc = 0);


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Expression* pred_;
  Expression* body_;

  Loop(Expression* pred, Expression* body, SourceLoc loc)
      : Expression(loc), pred_(pred), body_(body) {}
};

/// Cool expression block
class Block : public Expression {
 public:
  static Block* Create(Expressions* body, SourceLoc loc = 0);


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Expressions* body_;

  Block(Expressions* body, SourceLoc loc) : Expression(loc), body_(body) {}
};

/// Cool let expression with single variable declaration
class Let : public Expression {
 public:
  static Let* Create(Symbol* name, Symbol* decl_type, Expression* init, Expression* body,
                     SourceLoc loc = 0);


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Symbol* name_;
  Symbol* decl_type_;
  Expression* init_;
  Expression* body_;

  Let(Symbol* name, Symbol* decl_type, Expression* init, Expression* body, SourceLoc loc)
      : Expression(loc), name_(name), decl_type_(decl_type), init_(init), body_(body) {}
};

/// Cool case expression
class Kase : public Expression {
 public:
  static Kase* Create(Expression* input, KaseBranches* cases, SourceLoc loc = 0);


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Expression* input_;
  KaseBranches* cases_;

  Kase(Expression* input, KaseBranches* cases, SourceLoc loc)
      : Expression(loc), input_(input), cases_(cases) {}
};

/// Cool single case branch
class KaseBranch : public Expression {
 public:
  static KaseBranch* Create(Symbol* name, Symbol* decl_type, Expression* body, SourceLoc loc = 0);

  Symbol* decl_type() const { return decl_type_; }


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Symbol* name_;
  Symbol* decl_type_;
  Expression* body_;

  KaseBranch(Symbol* name, Symbol* decl_type, Expression* body, SourceLoc loc)
      : Expression(loc), name_(name), decl_type_(decl_type), body_(body) {}
};

/// Cool new operator
class Knew : public Expression {
 public:
  static Knew* Create(Symbol* name, SourceLoc loc = 0);


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Symbol* name_;

  Knew(Symbol* name, SourceLoc loc) : Expression(loc), name_(name) {}
};

/// Cool unary operator
class UnaryOperator : public Expression {
 public:
  enum UnaryKind { UO_Neg, UO_Not, UO_IsVoid };

  static UnaryOperator* Create(UnaryKind kind, Expression* input, SourceLoc loc = 0);


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  UnaryKind kind_;
  Expression* input_;

  UnaryOperator(UnaryKind kind, Expression* input, SourceLoc loc)
      : Expression(loc), kind_(kind), input_(input) {}
};

/// Cool binary operator
class BinaryOperator : public Expression {
 public:
  // We use a BO_ prefix to prevent conflicts with Bison defines
  enum BinaryKind { BO_Add, BO_Sub, BO_Mul, BO_Div, BO_LT, BO_EQ, BO_LE };

  static BinaryOperator* Create(BinaryKind kind, Expression* lhs, Expression* rhs,
                                SourceLoc loc = 0);

  const char* KindAsString() const;


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  BinaryKind kind_;
  Expression* lhs_;
  Expression* rhs_;

  BinaryOperator(BinaryKind kind, Expression* lhs, Expression* rhs, SourceLoc loc)
      : Expression(loc), kind_(kind), lhs_(lhs), rhs_(rhs) {}
};

/// Cool reference identifier
class Ref : public Expression {
 public:
  static Ref* Create(Symbol* name, SourceLoc loc = 0);


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  Symbol* name_;

  Ref(Symbol* name, SourceLoc loc) : Expression(loc), name_(name) {}
};

/// Placeholder node for the absence of an Expression
class NoExpr : public Expression {
 public:
  static NoExpr* Create(SourceLoc loc = 0);


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  NoExpr(SourceLoc loc) : Expression(loc) {}
};

/// Cool string literal
class StringLiteral : public Expression {
 public:
  static StringLiteral* Create(const StringEntry* value, SourceLoc loc = 0);
  static StringLiteral* Create(const std::string& string, SourceLoc loc = 0);
  static StringLiteral* Create(const char* string, std::size_t length, SourceLoc loc = 0);

  const std::string& value() const { return value_->value(); }


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

  friend std::ostream& operator<<(std::ostream& os, const StringLiteral* s) {
    return os << s->value();
  }

 protected:
  const StringEntry* value_;

  StringLiteral(const StringEntry* value, SourceLoc loc) : Expression(loc), value_(value) {}
};

/// Cool integer literal
class IntLiteral : public Expression {
 public:
  static IntLiteral* Create(const Int32Entry* value, SourceLoc loc = 0);
  static IntLiteral* Create(int32_t value, SourceLoc loc = 0);

  int32_t value() const { return value_->value(); }


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  const Int32Entry* value_;

  IntLiteral(const Int32Entry* value, SourceLoc loc) : Expression(loc), value_(value) {}
};

/// Cool boolean literal
class BoolLiteral : public Expression {
 public:
  static BoolLiteral* Create(bool, SourceLoc loc = 0);

  bool value() const { return value_; }


  void DumpTree(std::ostream& os, size_t level, bool with_types) const override;

 protected:
  const bool value_;

  BoolLiteral(bool value, SourceLoc loc) : Expression(loc), value_(value) {}
};

}  // namespace cool
