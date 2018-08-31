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

#include "ast_supp.h"

namespace cool {
// Forward declaration of AST Nodes
class ASTNode;
class Program;
class Klass;
typedef ASTNodeVector<Klass> Klasses;
class Feature;
typedef ASTNodeVector<Feature> Features;
class Method;
class Attr;
class Formal;
typedef ASTNodeVector<Formal> Formals;
class Expression;
typedef ASTNodeVector<Expression> Expressions;
class Assign;
class Dispatch;
class StaticDispatch;
class Cond;
class Loop;
class Block;
class Let;
class Kase;
class KaseBranch;
typedef ASTNodeVector<KaseBranch> KaseBranches;
class Knew;
class UnaryOperator;
class BinaryOperator;
class Ref;
class NoExpr;
class StringLiteral;
class IntLiteral;
class BoolLiteral;

/// Location (line number) in a source file
typedef std::size_t SourceLoc;

/// Name of a register
class Register;
}
