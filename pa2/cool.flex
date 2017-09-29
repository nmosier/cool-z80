/*
 * The scanner definition for COOL.
 */

/*
 * Code enclosed in %{ %} in the first section is copied verbatim to the
 * output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything from the skeleton.
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

#define YY_NO_UNPUT   /* keep g++ happy */
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
        YY_FATAL_ERROR("read() in flex scanner failed"); \
    }


/* String buffer for assembling string constants */
std::string string_buf;


extern cool::SourceLoc gCurrLineNo;

extern YYSTYPE cool_yylval;

/*
 * Add your own definitions here
 */

/* nested comments counter */
unsigned int comments_nestlevel = 0;

%}

    /* In this section, anything not Flex must be indented */

    /*
    * Define names for regular expressions here.
    */

    /* definition for nested comments: 'safe' because it can be followed by any (allowable) character, including *, (, and ) */
nested_safe [(*)]|(([[:print:]\t]{-}[(*])*[*]*(([[:print:]\t]{-}[(*)])|([(]+([[:print:]\t]{-}[(*])+)))*

    /* definitions for string constants */
str_escape [\\][[:print:][:space:]]
str_plainc [[:graph:][:blank:]]{-}[\\"]
str_body ({str_escape}|{str_plainc})*

    /* Define additional start conditions in addition to INITIAL (all rules without an explicit start condition) */
    /* start condition for string constant */
%x in_string
    /* start condition for nested comments */
%x nested_comment


    /* Automatically report coverage holes */
%option nodefault

%%

    /* whitespace */
[\n] { ++gCurrLineNo; }
[ \t\f\v\r]+ { /* Do nothing */ }

    /* single-line comments */
"--"[^\n]*[\n]    { ++gCurrLineNo; }

    /*
     *  Nested comments
     */
"*)"    {
    cool_yylval.error_msg = "Unmatched *)";
    return ERROR;
}
"(*"    { ++comments_nestlevel; BEGIN(nested_comment); }    /* now entering multi-line comment */

<nested_comment>{nested_safe}[\n]   {++gCurrLineNo; }
<nested_comment>{nested_safe}"(*"   { ++comments_nestlevel; } /* already in comment, so inc nest level */
<nested_comment>{nested_safe}"*)" {
    if (--comments_nestlevel == 0)
        BEGIN(INITIAL);
}
<nested_comment>{nested_safe}   {}  /* consume rest of comment if unmatched '*)' */
<nested_comment><<EOF>> {
    cool_yylval.error_msg = "EOF in comment";
    BEGIN(INITIAL);
    return ERROR;
}

    /*
     *  The multiple-character operators.
     */
"=>" { return (DARROW); }
"<-" { return ASSIGN; }
"<=" { return LE; }

    /* single character operators/tokens */
[+\-*/~<=(){};,:.@] { return *yytext; }

    /*
    * Keywords are case-insensitive except for the values true and false,
    * which must begin with a lower-case letter.
    */

    /* booleans */
"t"(?i:"rue") {
    cool_yylval.expression = cool::BoolLiteral::Create(true, gCurrLineNo);
    return BOOL_CONST;
}
"f"(?i:"alse") {
    cool_yylval.expression = cool::BoolLiteral::Create(false, gCurrLineNo);
    return BOOL_CONST;
}

    /* integer constants */
[0-9]+    {
    try {
        cool_yylval.expression = cool::IntLiteral::Create(std::stoi(std::string(yytext)), gCurrLineNo);
        return INT_CONST;
    } catch(std::out_of_range& e) {
        cool_yylval.error_msg = "Integer literal is out of range";
        return ERROR;
    }
    // 2147483647 is max
}

(?i:"class") { return CLASS; }
(?i:"else") { return ELSE; }
(?i:"fi") { return FI; }
(?i:"if") { return IF; }
(?i:"in") { return IN; }
(?i:"inherits") { return INHERITS; }
(?i:"isvoid") { return ISVOID; }
(?i:"let") { return LET; }
(?i:"loop") { return LOOP; }
(?i:"pool") { return POOL; }
(?i:"then") { return THEN; }
(?i:"while") { return WHILE; }
(?i:"case") { return CASE; }
(?i:"esac") { return ESAC; }
(?i:"new") { return NEW; }
(?i:"of") { return OF; }
(?i:"not") {return NOT; }

    /*
    *  String constants (C syntax, taken from lexdoc(1) )
    *  Escape sequence \c is accepted for all characters c. Except for
    *  \n \t \b \f, the result is c. (but note that 'c' can't be the NUL character)
    */

[\"]    { BEGIN(in_string); }

<in_string>{str_body}[\n] {
    cool_yylval.error_msg = "Unterminated string constant";
    BEGIN(INITIAL);
    ++gCurrLineNo;
    return ERROR;
}

<in_string>({str_body}|[\0])*[\0]{str_body}*["] {
    cool_yylval.error_msg = "String contains null character.";
    for (int i = 0; i < yyleng; ++i)
        if (yytext[i] == '\n')
            ++gCurrLineNo;
    BEGIN(INITIAL);
    return ERROR;
}

<in_string>{str_body}["]  {
    // pass thru yyltext & replace escape sequences, update newlines, etc
    int i, j;
    const char *special;
    const char *special_chars = "btnf";
    const char *special_conversions = "\b\t\n\f";
    for (i = j = 0; j < yyleng-1; ++i, ++j) {
        char c = yytext[j];

        if (c == '\\') {
            if ((special = strchr(special_chars, yytext[++j])) && *special != '\0') {
                    yytext[i] = special_conversions[special - special_chars];
            } else {
                yytext[i] = yytext[j];
                if (yytext[j] == '\n')
                    ++gCurrLineNo;
            }
        } else
            yytext[i] = yytext[j];
    }
    yytext[i] = '\0';
    if (i > 1024) {
        cool_yylval.error_msg = "String constant too long";
        BEGIN(INITIAL);
        return ERROR;
    } else {
        BEGIN(INITIAL);
        cool_yylval.expression = cool::StringLiteral::Create(yytext);
        return STR_CONST;
    }
}
    /* consume rest of string if EOF */
<in_string>{str_body} {}
<in_string><<EOF>> {
    cool_yylval.error_msg = "EOF in string constant";
    BEGIN(INITIAL);
    return ERROR;
}


    /* identifiers */
[[:lower:]][[:alnum:]_]* {
    cool::Symbol *entry = cool::gIdentTable.emplace(yytext, yyleng);
    cool_yylval.symbol = entry;
    return OBJECTID;
}
[[:upper:]][[:alnum:]_]* {
    cool::Symbol *entry = cool::gIdentTable.emplace(yytext, yyleng);
    cool_yylval.symbol = entry;
    return TYPEID;
}

    /* catch invalid characters */
[[:print:]] {
    cool_yylval.error_msg = yytext;
    return ERROR;
}

    /* catch EOF */
<<EOF>> { yyterminate(); }
%%
