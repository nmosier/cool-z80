/*
 *  A scanner definition for COOL token stream
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
#include "cool_parse.h"
#include "stringtab.h"
#include "utilities.h"

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1024

#define YY_NO_UNPUT
#define yywrap() 1
#define YY_SKIP_YYWRAP

extern std::istream* gInputStream;

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
    gInputStream->read((char*)buf, max_size); \
    if ((result = gInputStream->gcount()) < 0) { \
        YY_FATAL_ERROR( "read() in flex scanner failed"); \
    }

std::string string_buf; /* to assemble string constants */

extern cool::SourceLoc gCurrLineNo;
extern const char* gCurrFilename;

static int prevstate;

%}

WHITESPACE [ \t\b\f\v\r\n]

SYM     [A-Za-z][_A-Za-z0-9]*
SINGLE  [+/\-*=<\.~,;:()@\}\{]

%x TOKEN STR ERR INT BOOL TYPSYM OBJSYM STRING
%%

<INITIAL,TOKEN,STR,ERR,INT,BOOL,TYPSYM,OBJSYM>{WHITESPACE}+ {}

#[0-9]+              { gCurrLineNo = atoi(yytext + 1); BEGIN(TOKEN); }
#name{WHITESPACE}*\" { string_buf.clear(); prevstate = INITIAL; BEGIN(STRING); }

<TOKEN>{
  CLASS	     { BEGIN(INITIAL); return CLASS; }
  ELSE       { BEGIN(INITIAL); return ELSE; }
  FI         { BEGIN(INITIAL); return FI; }
  IF         { BEGIN(INITIAL); return IF; }
  IN         { BEGIN(INITIAL); return IN; }
  INHERITS   { BEGIN(INITIAL); return INHERITS; }
  LET        { BEGIN(INITIAL); return LET; }
  LOOP       { BEGIN(INITIAL); return LOOP; }
  POOL       { BEGIN(INITIAL); return POOL; }
  THEN       { BEGIN(INITIAL); return THEN; }
  WHILE      { BEGIN(INITIAL); return WHILE; }
  ASSIGN     { BEGIN(INITIAL); return ASSIGN; }
  CASE       { BEGIN(INITIAL); return CASE; }
  ESAC       { BEGIN(INITIAL); return ESAC; }
  OF         { BEGIN(INITIAL); return OF; }
  DARROW     { BEGIN(INITIAL); return DARROW; }
  NEW        { BEGIN(INITIAL); return NEW; }
  LE         { BEGIN(INITIAL); return LE; }
  NOT        { BEGIN(INITIAL); return NOT; }
  ISVOID     { BEGIN(INITIAL); return ISVOID; }

  STR_CONST  { BEGIN(STR); }
  INT_CONST  { BEGIN(INT); }
  BOOL_CONST { BEGIN(BOOL); }
  TYPEID     { BEGIN(TYPSYM); }
  OBJECTID   { BEGIN(OBJSYM); }
  ERROR	     { BEGIN(ERR); }

  \'{SINGLE}\' { BEGIN(INITIAL); return *(yytext + 1); }

  .	{ YY_FATAL_ERROR("unmatched text in token lexer; token expected"); }
}

<STR>\"	{ string_buf.clear(); prevstate = STR; BEGIN(STRING); }
<STR>.  { YY_FATAL_ERROR("unmatched text in token lexer; string constant expected"); }

<ERR>\"	{ string_buf.clear(); prevstate = ERR; BEGIN(STRING); }
<ERR>.  { YY_FATAL_ERROR("unmatched text in token lexer; error message expected"); }

<INT>[0-9][0-9]* {
  try {
    yylval.expression = cool::IntLiteral::Create(std::stoi(std::string(yytext, yytext+yyleng)), gCurrLineNo);
    BEGIN(INITIAL);
    return (INT_CONST);
  } catch (std::out_of_range& e) {
    YY_FATAL_ERROR("Integer literal is out of range");
  }
}
<INT>. { YY_FATAL_ERROR("unmatched text in token lexer; int constant expected"); }

<BOOL>true	{ yylval.expression = cool::BoolLiteral::Create(true, gCurrLineNo); BEGIN(INITIAL); return BOOL_CONST; }
<BOOL>false	{ yylval.expression = cool::BoolLiteral::Create(false, gCurrLineNo); BEGIN(INITIAL); return BOOL_CONST; }
<BOOL>.     { YY_FATAL_ERROR("unmatched text in token lexer; bool constant expected"); }

<TYPSYM>{SYM} {
  yylval.symbol = cool::gIdentTable.emplace(yytext, yyleng);
  BEGIN(INITIAL);
  return (TYPEID);
}
<TYPSYM>. { YY_FATAL_ERROR("unmatched text in token lexer; type symbol expected"); }

<OBJSYM>{SYM} {
  yylval.symbol = cool::gIdentTable.emplace(yytext, yyleng);
  BEGIN(INITIAL);
  return (OBJECTID);
}
<OBJSYM>. { YY_FATAL_ERROR("unmatched text in token lexer; object symbol expected"); }

<STRING>{
  \" {
    /* We saw a closing quote, string literal is complete */
    BEGIN(INITIAL);
    if (prevstate == STR) {
      yylval.expression = cool::StringLiteral::Create(string_buf, gCurrLineNo);
      return (STR_CONST);
    } else if (prevstate == ERR) {
      yylval.error_msg = strdup(string_buf.c_str());
      return ERROR;
    } else if (prevstate == INITIAL) {
      gCurrFilename = strdup(string_buf.c_str());
    } else {
      YY_FATAL_ERROR("unknown state");
    }
  }

  \\n	 { string_buf.push_back('\n'); }
  \\t	 { string_buf.push_back('\t'); }
  \\b	 { string_buf.push_back('\b'); }
  \\f	 { string_buf.push_back('\f'); }
  \\\\ { string_buf.push_back('\\'); }
  \\\" { string_buf.push_back('\"'); }

  \\[0-3][0-7][0-7] {
    /* Unprintable characters are represented as octal numbers */
    string_buf.push_back(strtol(yytext+1, 0, 8));
  }

  . { string_buf.push_back(yytext[0]); }
}

<<EOF>> { yyterminate(); }

. { YY_FATAL_ERROR("unmatched text in token lexer; line number expected"); }

%%
