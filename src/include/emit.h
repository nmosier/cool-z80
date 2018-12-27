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

///////////////////////////////////////////////////////////////////////
//
//  Assembly Code Naming Conventions:
//
//     Dispatch table            <classname>_dispTab
//     Method entry point        <classname>.<method>
//     Class init code           <classname>_init
//     Abort method entry        <classname>.<method>.Abort
//     Prototype object          <classname>_protObj
//     Integer constant          int_const<Symbol>
//     String constant           str_const<Symbol>
//
///////////////////////////////////////////////////////////////////////

#define WORD_SIZE    2
#define LOG_WORD_SIZE 2     // For logical shifts

// File names
#define DISPTAB_PATH         "disptab.z80"

// Global names
#define CLASSNAMETAB         "class_nameTab"
#define CLASSOBJTAB          "class_objTab"
#define INTTAG               "_int_tag"
#define BOOLTAG              "_bool_tag"
#define STRINGTAG            "_string_tag"
#define HEAP_START           "heap_start"
#define INHERITANCE_TREE	 "inheritance_tree"

// Naming conventions
#define DISPTAB_SUFFIX       "_dispTab"
#define DISPENT_PREFIX       "_dispTab."
#define METHOD_SEP           "."
#define CLASSINIT_SUFFIX     "_init"
#define PROTOBJ_SUFFIX       "_protObj"
#define OBJECTPROTOBJ        "Object_protObj"
#define INTCONST_PREFIX      "int_const"
#define STRCONST_PREFIX      "str_const"
#define BOOLCONST_PREFIX     "bool_const"


#define EMPTYSLOT            0
#define LABEL                ":\n"

#define STRINGNAME (char *) "String"
#define INTNAME    (char *) "Int"
#define BOOLNAME   (char *) "Bool"
#define MAINNAME   (char *) "Main"

//
// information about object headers
//
#define DEFAULT_OBJFIELDS 3
#define TAG_OFFSET 0
#define SIZE_OFFSET 1
#define DISPTABLE_OFFSET 2

#define STRING_SLOTS      1
#define INT_SLOTS         1
#define BOOL_SLOTS        1

// DIRECTIVES
#define DW            "\t.dw\t"
#define DB            "\t.db\t"

#define BREAK		  "\tBREAK"

// PREPROCESSOR
#define INCLUDE	"#include\t"
#define DEFINE    "#define\t"

//
// register names
//
// Register const ZERO = "$zero";		// Zero register 
// Register const ACC  = "$a0";		// Accumulator 
// Register const A1   = "$a1";		// For arguments to prim functions
// Register const SELF = "$s0";		// Pointer to self (callee saves)
// Register const T1   = "$t1";		// Temporary 1 
// Register const T2   = "$t2";		// Temporary 2 
// Register const T3 	= "$t3";		// Temporary 3
// Register const T5	= "$t5";		// Temporary 5 -- not destroyed by support code functions
// Register const SP   = "$sp";		// Stack pointer 
// Register const FP   = "$fp";		// Frame pointer 
// Register const RA   = "$ra";		// Return address


//
// Opcodes
//
#define CALL "\tcall\t"
#define JR   "\tjr\t"
#define JP   "\tjp\t"
#define RET	 "\tret\t"

#define PUSH "\tpush\t"
#define POP  "\tpop\t"

#define LD   "\tld\t"

#define EX   "\tex\t"

#define SCF  "\tscf"
#define CCF  "\tccf"
#define BIT  "\tbit\t"

#define CP   "\tcp\t"
#define DJNZ "\tdjnz\t"
#define RLA  "\trla"
#define RLCA "\trlca"
#define RL   "\trl\t"

#define NEG  "\tneg\t"
#define CPL	 "\tcpl\t"
#define ADD  "\tadd\t"
#define ADC  "\tadc\t"
#define SUB  "\tsub\t"
#define SBC  "\tsbc\t"
#define INC  "\tinc\t"
#define DEC  "\tdec\t"

#define SLA  "\tsla\t"
#define SRA  "\tsra\t"
#define SRL  "\tsrl\t"
#define AND	 "\tand\t"
#define OR   "\tor\t"
#define XOR  "\txor\t"
