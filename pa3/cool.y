/*
 * Parser definition for the COOL language.
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

  #include "ast.h"
  #include "stringtab.h"
  #include "utilities.h"

  // Locations
  #define YYLTYPE cool::SourceLoc   // The type of locations
  #define cool_yylloc gCurrLineNo   // Use the gCurrLineNo from the lexer for the location of tokens

  // The default action for locations. Use the location of the first
  // terminal/non-terminal.
  #define YYLLOC_DEFAULT(Cur, Rhs, N)         \
    (Cur) = (N) ? YYRHSLOC(Rhs, 1) : YYRHSLOC(Rhs, 0);

  extern const char *gCurrFilename; // Current file being parsed

  void yyerror(const char *s);  // Called for each parse error
  extern int yylex();           // Entry point to the lexer/

  cool::Program* gASTRoot;	    // The result of the parsing
  int omerrs = 0;               // Number of lexing or parsing errors

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
  const char *error_msg;
}

/* 
Declare the terminals; a few have types for associated lexemes.
The token ERROR is never used in the parser; thus, it is a parse
error when the lexer returns it.
 */

%token CLASS 258
%token INHERITS 259
%token IF 260
%token THEN 261
%token ELSE 262
%token FI 263
%token IN 264
%token LET 265
%token WHILE 266
%token LOOP 267
%token POOL 268
%token CASE 269
%token ESAC 270
%token OF 271
%token DARROW 272
%token NEW 273
%token ISVOID 274
%token ASSIGN 275
%token NOT 276
%token LE 277
%token <expression> STR_CONST 278
%token <expression> INT_CONST 279
%token <expression> BOOL_CONST 280
%token <symbol> TYPEID 281
%token <symbol> OBJECTID 282
%token ERROR 283
%token LET_STMT 284

/* The terminal symbol error is reserved for error recovery. */

/* Don't change anything above this line, or your parser will NOT work */
/* --------------------------------------------------------------------*/

/* Declare types for the grammar's non-terminals. */
%type <program> program
%type <klasses> class_list
%type <klass> class
%type <features> optional_feature_list
%type <features> feature_list
%type <feature> feature
%type <formals> optional_formal_list
%type <formals> formal_list
%type <formal> formal
%type <expression> expression
%type <kase_branches> case_branch_list
%type <kase_branch> case_branch
%type <expression> let_statement_recursive
%type <expression> optional_initializer
%type <expressions> expression_list
%type <expressions> optional_actuals_list
%type <expressions> actuals_list

/* Precedence declarations (in reverse order of precendence). */

%right IN

%right ASSIGN
%nonassoc NOT
%nonassoc LE '<' '='
%left '+' '-'
%left '*' '/'
%nonassoc ISVOID
%nonassoc '~'
%nonassoc '@'
%nonassoc '.'

%%

program	:
  class_list {
    /* Ensure bison computes location information */
    @$ = @1;
    gASTRoot = cool::Program::Create($1, @1); // Save AST root in a global variable for access by programs
  }
  | error { gASTRoot = cool::Program::Create(cool::Klasses::Create()); }
  ;

class_list :
  class                   { $$ = cool::Klasses::Create($1); }
  | error ';'             { $$ = cool::Klasses::Create(); } // Error in the 1st class
  | class_list class      { $$ = ($1)->push_back($2); }
  | class_list error ';'  { $$ = $1; }
  ;


class :
  CLASS TYPEID '{' optional_feature_list '}' ';' {
    /* If no parent class is specified, the class inherits from Object */
    $$ = cool::Klass::Create($2, cool::gIdentTable.emplace("Object"), $4, cool::StringLiteral::Create(gCurrFilename), @1);
  }
  | CLASS TYPEID INHERITS TYPEID '{' optional_feature_list '}' ';' {
    $$ = cool::Klass::Create($2, $4, $6, cool::StringLiteral::Create(gCurrFilename), @1);
  }
  ;

/* Feature list may be empty, but no empty features in list. You will need to flesh this out further. */
optional_feature_list :
  /* empty */ { $$ = cool::Features::Create(); }
  | feature_list { $$ = $1; }	
  ;

feature_list :
  feature ';'	{ $$ = cool::Features::Create($1); }
  | error ';'	{ $$ = cool::Features::Create(); }
  | feature_list feature ';' { $$ = ($1)->push_back($2); }
  | feature_list error ';'	{ $$ = $1; }
  ;

feature:
	OBJECTID ':' TYPEID ASSIGN expression { $$ = cool::Attr::Create($1, $3, $5, @1); }
	| OBJECTID ':' TYPEID	{ $$ = cool::Attr::Create($1, $3, cool::NoExpr::Create(), @1); }
	| OBJECTID '(' optional_formal_list ')' ':' TYPEID '{' expression '}'
		{ $$ = cool::Method::Create($1, $3, $6, $8, @1); }
	;

optional_formal_list:
	/* empty */ { $$ = cool::Formals::Create(); }
	| formal_list	{ $$ = $1; }
	;

formal_list:
	formal	{ $$ = cool::Formals::Create($1); }
	| formal_list ',' formal	 { $$ = ($1)->push_back($3); }
	;

formal:
	OBJECTID ':' TYPEID	{ $$ = cool::Formal::Create($1, $3, @1); }
	;

expression:
	BOOL_CONST	{ $$ = $1; }
	| INT_CONST { $$ = $1; }
	| STR_CONST { $$ = $1; }
	| OBJECTID	{ $$ = cool::Ref::Create($1, @1); }
	| '(' expression ')'	{ $$ = $2; }
	| NOT expression
		{ $$ = cool::UnaryOperator::Create(cool::UnaryOperator::UO_Not, $2, @1); }
	| '~' expression
		{ $$ = cool::UnaryOperator::Create(cool::UnaryOperator::UO_Neg, $2, @1); }
	| expression '=' expression
		{ $$ = cool::BinaryOperator::Create(cool::BinaryOperator::BO_EQ, $1, $3, @3); }
	| expression LE expression
		{ $$ = cool::BinaryOperator::Create(cool::BinaryOperator::BO_LE, $1, $3, @3); }
	| expression '<' expression
		{ $$ = cool::BinaryOperator::Create(cool::BinaryOperator::BO_LT, $1, $3, @3); }
	| expression '/' expression
		{ $$ = cool::BinaryOperator::Create(cool::BinaryOperator::BO_Div, $1, $3, @3); }
	| expression '*' expression
		{ $$ = cool::BinaryOperator::Create(cool::BinaryOperator::BO_Mul, $1, $3, @3); }
	| expression '-' expression
		{ $$ = cool::BinaryOperator::Create(cool::BinaryOperator::BO_Sub, $1, $3, @3); }
	| expression '+' expression
		{ $$ = cool::BinaryOperator::Create(cool::BinaryOperator::BO_Add, $1, $3, @3); }
	| ISVOID expression
		{ $$ = cool::UnaryOperator::Create(cool::UnaryOperator::UO_IsVoid, $2, @1); }
	| NEW TYPEID
		{ $$ = cool::Knew::Create($2, @1); }
	| CASE expression OF case_branch_list ESAC
		{ $$ = cool::Kase::Create($2, $4, @1); }
	| LET let_statement_recursive { $$ = $2; }
	| '{' expression_list '}'
		{ $$ = cool::Block::Create($2, @1); }
	| WHILE expression LOOP expression POOL
		{ $$ = cool::Loop::Create($2, $4, @1); }
	| IF expression THEN expression ELSE expression FI
		{ $$ = cool::Cond::Create($2, $4, $6, @1); }
	| OBJECTID '(' optional_actuals_list ')'
		{ $$ = cool::Dispatch::Create(cool::Ref::Create(cool::gIdentTable.emplace("self"), @1), $1, $3, @1); }
	| expression '.' OBJECTID '(' optional_actuals_list ')'
		{ $$ = cool::Dispatch::Create($1, $3, $5, @1); }
	| expression '@' TYPEID '.' OBJECTID '(' optional_actuals_list ')'
		{ $$ = cool::StaticDispatch::Create($1, $3, $5, $7, @1); }
	| OBJECTID ASSIGN expression
		{ $$ = cool::Assign::Create($1, $3, @1); }
	;
	
case_branch_list:
	case_branch							{ $$ = cool::KaseBranches::Create($1); }
	| error								{ $$ = cool::KaseBranches::Create(); }
	| case_branch_list case_branch	{ $$ = ($1)->push_back($2); }
	| case_branch_list error			{ $$ = $1; }
	;

case_branch:
	OBJECTID ':' TYPEID DARROW expression ';'	{ $$ = cool::KaseBranch::Create($1, $3, $5, @1); }
	;


let_statement_recursive:
	OBJECTID ':' TYPEID optional_initializer IN expression
		{ $$ = cool::Let::Create($1, $3, $4, $6, @1); }
	| OBJECTID ':' TYPEID optional_initializer ',' let_statement_recursive
		{ $$ = cool::Let::Create($1, $3, $4, $6, @1); }
	| error ',' let_statement_recursive
		{ $$ = $3; }
	| error IN expression
		{ $$ = $3; }
	;

optional_initializer:
	/* no initializer */	{ $$ = cool::NoExpr::Create(); }
	| ASSIGN expression		{ $$ = $2; }
	;
	

expression_list:
	expression ';'	{ $$ = cool::Expressions::Create($1); }
	| expression_list expression ';' { $$ = ($1)->push_back($2); }
	/* error catching */
	| error ';'	{ $$ = cool::Expressions::Create(); }
	| expression_list error ';' { $$ = $1; };
	;
	
optional_actuals_list:
	/* empty list */	{ $$ = cool::Expressions::Create(); }
	| actuals_list		{ $$ = $1; }
	;

actuals_list:
	expression			{ $$ = cool::Expressions::Create($1); }
	| actuals_list ',' expression	{ $$ = ($1)->push_back($3); }
	;

/* end of grammar */
%%

/* This function is called automatically when Bison detects a parse error. */
void yyerror(const char *s) {
  extern cool::SourceLoc gCurrLineNo;

  std::cerr << "\"" << gCurrFilename << "\", " << "line " << gCurrLineNo << ": " << s << " at or near ";
  cool::print_cool_token(std::cerr, yychar);
  std::cerr << std::endl;

  if (++omerrs > 50) {
    std::cerr << "More than 50 errors" << std::endl;
    exit(1);
  }
}


