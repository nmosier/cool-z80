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


void Semant(Program* program) {
  // Initialize error tracker (and reporter)
  SemantError error(std::cerr);

  // Initialize Cool "built-in" symbols
  InitCoolSymbols();


  // Halt program with non-zero exit if there are semantic errors
  if (error.errors()) {
    std::cerr << "Compilation halted due to static semantic errors." << std::endl;
    exit(1);
  }
}

} // namespace cool

