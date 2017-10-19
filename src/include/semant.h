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

}



