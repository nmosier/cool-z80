/*
 *  A scanner definition for COOL ASTs.
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

#include <string>
#include <istream>

#include "ast.h"
#include "ast_parse.h"
#include "stringtab.h"
#include "utilities.h"

/* The compiler assumes these identifiers. */
#define yylval ast_yylval
#define yylex  ast_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1024

#define YY_NO_UNPUT   /* keep g++ happy */
#define yywrap() 1
#define YY_SKIP_YYWRAP

extern std::istream* gInputStream;


#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	gInputStream->read((char*)buf, max_size); \
    if ((result = gInputStream->gcount()) < 0) { \
        YY_FATAL_ERROR( "read() in flex scanner failed"); \
    }

std::string string_buf;

extern YYSTYPE ast_yylval;
YYSTYPE cool_yylval;  /* needed to link ast code with utilities.cc */

extern cool::SourceLoc gCurrLineNo;

%}

WHITESPACE [ \t\b\f\v\r\n]

SYM   [A-Za-z][_A-Za-z0-9]*

%x STRING
%%

{WHITESPACE}+	{}
[0-9][0-9]* {
  try {
    yylval.expression = cool::IntLiteral::Create(std::stoi(std::string(yytext, yytext+yyleng)), yylval.lineno);
    return (INT_CONST);
  } catch (std::out_of_range& e) {
    YY_FATAL_ERROR("Integer literal is out of range");
  }
}

#[0-9]* { yylval.lineno = gCurrLineNo = atoi(yytext + 1); return (LINENO); }

_program { return(PROGRAM); }
_class   { return(CLASS); }
_method  { return(METHOD); }
_attr    { return(ATTR); }
_formal  { return(FORMAL); }
_branch  { return(BRANCH); }
_assign  { return(ASSIGN); }
_static_dispatch	{ return(STATIC_DISPATCH); }
_dispatch { return(DISPATCH); }
_cond     { return(COND); }
_loop     { return(LOOP); }
_typcase  { return(TYPCASE); }
_block    { return(BLOCK); }
_let      { return(LET); }
_plus     { return(PLUS); }
_sub      { return(SUB); }
_mul      { return(MUL); }
_divide   { return(DIVIDE); }
_neg      { return(NEG); }
_lt       { return(LT); }
_eq       { return(EQ); }
_leq      { return(LEQ); }
_comp     { return(COMP); }
_int      { return(INT); }
_string   { return(STR); }
_bool     { return(BOOL); }
_new      { return(NEW); }
_isvoid   { return(ISVOID); }
_no_expr  { return(NO_EXPR); }
_no_type  { return(NO_TYPE); }
_object   { return(OBJECT); }

[:()]     { return(*yytext); }


{SYM}	{ yylval.symbol = cool::gIdentTable.emplace(yytext, yyleng); return(ID); }


 /*
  *  String constants (C syntax, taken from lexdoc(1) )
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  */

\" { string_buf.clear();  BEGIN(STRING); }



<STRING>{
  \" {
    /* We saw closing quote, string literal is complete */
    BEGIN(INITIAL);
    yylval.string_literal = cool::StringLiteral::Create(string_buf, yylval.lineno);
    return (STR_CONST);
  }

  \\n	 { string_buf.push_back('\n'); }
  \\t	 { string_buf.push_back('\t'); }
  \\b	 { string_buf.push_back('\b'); }
  \\f	 { string_buf.push_back('\f'); }
  \\\\ { string_buf.push_back('\\'); }
  \\\" { string_buf.push_back('\"'); }

  \\[0-9]* {
    /* unprintable characters are represented as octal numbers */
    string_buf.push_back(strtol(yytext+1, 0, 8));
  }

  . { string_buf.push_back(yytext[0]); }
}

<<EOF>>	{ yyterminate(); }

%%
