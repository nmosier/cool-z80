/*
 *  ast.y
 *  Parser definition for reading Cool abstract syntax trees.
 */
%{
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

  #include "ast.h"
  #include "stringtab.h"
  #include "utilities.h"

  cool::SourceLoc gCurrLineNo = 0;  /* debugging, current line for input file */

  void ast_yyerror(const char *);
  extern int yylex();           /* the entry point to the lexer  */

  cool::Program* gASTRoot;      /* the result of the parse  */
  int omerrs = 0;               /* number of errors in lexing and parsing */

  using BinaryKind = cool::BinaryOperator::BinaryKind;
  using UnaryKind  = cool::UnaryOperator::UnaryKind;
%}

/* A union of all the types that can be the result of parsing actions. */
%union {
  cool::Program* program;
  cool::Klass* klass;
  cool::Klasses* klasses;
  cool::Feature* feature;
  cool::Features* features;
  cool::Formal* formal;
  cool::Formals* formals;
  cool::Expression* expression;
  cool::Expressions* expressions;
  cool::KaseBranch* kase_branch;
  cool::KaseBranches* kase_branches;
  cool::Symbol* symbol;
  cool::StringLiteral* string_literal;
  int lineno;
}

/* Declare types for the grammar's non-terminals. */
%type <program> program
%type <klasses> class_list
%type <klass> class
%type <features> feature_list optional_feature_list
%type <feature> feature
%type <formals> formals formal_list
%type <formal> formal
%type <expression> expr_aux expr
%type <expressions> actuals expr_list
%type <kase_branches> case_list
%type <kase_branch> case_branch


/* 
 Declare the terminals; a few have types for associated lexemes.
 The token ERROR is never used in the parser; thus, it is a parse
 error when the lexer returns it.
*/
%token PROGRAM CLASS METHOD ATTR FORMAL BRANCH ASSIGN STATIC_DISPATCH DISPATCH 
%token COND LOOP TYPCASE BLOCK LET PLUS SUB MUL DIVIDE NEG LT EQ LEQ
%token COMP INT STR BOOL NEW ISVOID NO_EXPR OBJECT NO_TYPE
%token <string_literal> STR_CONST
%token <expression> INT_CONST
%token <symbol> ID
%token <lineno> LINENO


%%
/* 
   Save the root of the abstract syntax tree in a global variable.
*/
program	:
  nothing LINENO PROGRAM class_list { gASTRoot = cool::Program::Create($4, $2); }
  | nothing { exit(1); }
  ;

// Reset the line number to 1 each time we start a new program
nothing : { gCurrLineNo = 1; } ;

class_list :
  class { $$ = cool::Klasses::Create($1); }
  | class_list class { $$ = ($1)->push_back($2); }
  ;

class	:
  LINENO CLASS ID ID STR_CONST '(' optional_feature_list ')' {
    $$ = cool::Klass::Create($3,$4,$7,$5,$1);
  }
	;

/* Feature list may be empty, but no empty features in list. */
optional_feature_list :
  /* empty */ { $$ = cool::Features::Create(); }
  | feature_list { $$ = $1; }
  ;

feature_list :
  feature { $$ = cool::Features::Create($1); }
  | feature_list feature { $$ = ($1)->push_back($2); }
  ;

feature :
  LINENO METHOD ID formals ID expr { $$ = cool::Method::Create($3, $4, $5, $6, $1); }
  | LINENO ATTR ID ID expr { $$ = cool::Attr::Create($3, $4, $5, $1); }
  ;

formals	:
  /* empty */	{ $$ = cool::Formals::Create(); }
  | formal_list { $$ = $1; }
  ;

formal_list :
  formal {  $$ = cool::Formals::Create($1); }
  | formal_list formal { $$ = ($1)->push_back($2); }
  ;

formal :
  LINENO FORMAL ID ID { $$ = cool::Formal::Create($3, $4, $1); }
  ;

expr :
  expr_aux ':' ID { $$ = $1; $$->set_type($3); }
  | expr_aux ':' NO_TYPE { $$ = $1; }
  ;

expr_aux :
  LINENO ASSIGN ID expr { $$ = cool::Assign::Create($3, $4, $1); }
  | LINENO STATIC_DISPATCH expr ID ID actuals { $$ = cool::StaticDispatch::Create($3, $4, $5, $6, $1); }
  | LINENO DISPATCH expr ID actuals { $$ = cool::Dispatch::Create($3, $4, $5, $1); }
  | LINENO COND expr expr expr { $$ = cool::Cond::Create($3, $4, $5, $1); }
  | LINENO LOOP expr expr {  $$ = cool::Loop::Create($3, $4, $1); }
  | LINENO BLOCK expr_list { $$ = cool::Block::Create($3, $1); }
  | LINENO LET ID ID expr expr { $$ = cool::Let::Create($3, $4, $5, $6, $1); }
  | LINENO TYPCASE expr case_list { $$ = cool::Kase::Create($3, $4, $1); }
  | LINENO NEW ID { $$ = cool::Knew::Create($3, $1); }
  | LINENO ISVOID expr { $$ = cool::UnaryOperator::Create(UnaryKind::UO_IsVoid, $3, $1); }
  | LINENO PLUS expr expr { $$ = cool::BinaryOperator::Create(BinaryKind::BO_Add, $3, $4, $1); }
  | LINENO SUB expr expr { $$ = cool::BinaryOperator::Create(BinaryKind::BO_Sub, $3, $4, $1); }
  | LINENO MUL expr expr { $$ = cool::BinaryOperator::Create(BinaryKind::BO_Mul, $3, $4, $1); }
  | LINENO DIVIDE expr expr { $$ = cool::BinaryOperator::Create(BinaryKind::BO_Div, $3, $4, $1); }
  | LINENO NEG expr { $$ = cool::UnaryOperator::Create(UnaryKind::UO_Neg, $3, $1); }
  | LINENO LT expr expr { $$ = cool::BinaryOperator::Create(BinaryKind::BO_LT, $3, $4, $1); }
  | LINENO EQ expr expr { $$ = cool::BinaryOperator::Create(BinaryKind::BO_EQ, $3, $4, $1); }
  | LINENO LEQ expr expr { $$ = cool::BinaryOperator::Create(BinaryKind::BO_LE, $3, $4, $1); }
  | LINENO COMP expr { $$ = cool::UnaryOperator::Create(UnaryKind::UO_Not, $3, $1); }
  | LINENO INT INT_CONST { $$ = $3; }
  | LINENO STR STR_CONST { $$ = $3; }
  | LINENO BOOL INT_CONST {
    if (static_cast<cool::IntLiteral*>($3)->value()) {
      $$ = cool::BoolLiteral::Create(true, $1);
    } else {
      $$ = cool::BoolLiteral::Create(false, $1);
    }
  }
  | LINENO OBJECT ID { $$ = cool::Ref::Create($3, $1); }
  | LINENO NO_EXPR { $$ = cool::NoExpr::Create($1); }
  ;

actuals	:
  '(' ')' {  $$ = cool::Expressions::Create(); }
  | '(' expr_list ')'	{ $$ = $2; }
  ;

expr_list
  : expr { $$ = cool::Expressions::Create($1); }
  | expr_list expr { $$ = ($1)->push_back($2); }
  ;

case_list :
  case_branch { $$ = cool::KaseBranches::Create($1); }
  | case_list case_branch { $$ = ($1)->push_back($2);}
  ;

case_branch :
  LINENO BRANCH ID ID expr { $$ = cool::KaseBranch::Create($3, $4, $5, $1); }
  ;


/* end of grammar */
%%

void ast_yyerror(const char *msg) {
 std::cerr << "Error in ast parsing (line " << gCurrLineNo << "): "
           << msg
           << std::endl;
 exit(1);
}

