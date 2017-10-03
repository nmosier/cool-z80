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

#include <vector>

namespace cool {

/// Vector-like type for collections of ASTNode pointers
/// |tparam Elem Underlying element type (not a pointer type)
///
/// A thin wrapper around std::vector for maintaining vectors of ASTNode pointers. Adds
/// factory methods to faciliate AST creation during parsing.
template <class Elem>
class ASTNodeVector {
  typedef std::vector<Elem*> Data;

 public:
  typedef typename Data::size_type size_type;
  typedef typename Data::iterator iterator;
  typedef typename Data::const_iterator const_iterator;
  typedef typename Data::reverse_iterator reverse_iterator;
  typedef typename Data::const_reverse_iterator const_reverse_iterator;

  /// @}
  /// @name Factory methods
  /// @{
  static ASTNodeVector* Create() { return new ASTNodeVector(); }
  static ASTNodeVector* Create(Elem* elem) { return new ASTNodeVector({elem}); }
  static ASTNodeVector* Create(std::initializer_list<Elem*> list) {
    return new ASTNodeVector(list);
  }

  Elem* at(size_type i) { return data_.at(i); }
  Elem* back() { return data_.back(); }

  /// @}
  /// @name Iterators
  /// @{
  const_iterator begin() const { return data_.begin(); }
  const_iterator end() const { return data_.end(); }

  const_reverse_iterator rbegin() const { return data_.rbegin(); }
  const_reverse_iterator rend() const { return data_.rend(); }

  size_type size() const { return data_.size(); }

  /// push_back - Append elem to ASTNodeVector returning this ASTNodeVector
  ASTNodeVector* push_back(Elem* elem) {
    data_.push_back(elem);
    return this;
  }

  /// Insert nodes from other container into this ASTNodeVector
  /// \param elems Other ASTNodeVector
  ASTNodeVector* push_back(const ASTNodeVector* elems) {
    data_.insert(data_.end(), elems->begin(), elems->end());
    return this;
  }

 private:
  std::vector<Elem*> data_;

  // ASTNodeVector should only be created with factory methods
  ASTNodeVector() {}
  ASTNodeVector(std::initializer_list<Elem*> list) : data_(list) {}
};
}