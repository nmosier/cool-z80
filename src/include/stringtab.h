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

#include <string>
#include <vector>
#include <unordered_map>
#include <iosfwd>

namespace cool {

/// A string-like class that references another string object.
class StringRef {
  // Adapted from LLVM StringRef class
 private:
  const char* data_ = nullptr;
  std::size_t length_ = 0;

 public:
  StringRef() = delete;
  StringRef(std::nullptr_t) = delete;
  constexpr StringRef(const char* data, std::size_t length)
      : data_(data), length_(length) {}
  StringRef(const char* string);
  StringRef(const std::string& string)
      : data_(string.data()), length_(string.size()) {}

  std::size_t size() const { return length_; }

  const char* data() const { return data_; }

  bool operator==(const StringRef& rhs) const;
};

/// Symbol table. Maintain a single instance of Elem objects.
/// \tparam Elem Element type
template <class Elem>
class SymbolTable {
  typedef typename Elem::KeyType Key;
  typedef std::unordered_map<Key, std::unique_ptr<Elem>> TableType;

 public:
  typedef typename TableType::key_type key_type;
  typedef typename TableType::mapped_type mapped_type;
  typedef typename TableType::value_type value_type;
  typedef typename TableType::size_type size_type;
  typedef typename TableType::const_iterator const_iterator;

  /// Returns boolean indicating if Elem in table.
  /// \param elem Elem to query
  /// \return
  bool has(const Elem* elem) const {
    return entries_.find(elem->key()) != entries_.end();
  }
  bool has(Elem* elem) const {
    return entries_.find(elem->key()) != entries_.end();
  }

  template <class... Args>
  bool has(Args&&... args) const {
    return entries_.find(Key(std::forward<Args>(args)...)) != entries_.end();
  }

  /// Retrieve entry with key created from args.
  /// \param args Arguments to forward to create Elem (and corresponding key)
  /// \return Pointer to element or nullptr if not found.
  template <class... Args>
  Elem* lookup(Args&&... args) const {
    auto r = entries_.find(Key(std::forward<Args>(args)...));
    return (r != entries_.end()) ? r->second.get() : nullptr;
  }

  /// Emplace element constructed from args in table
  /// \param args Arguments to forward to create Elem (and corresponding Key)
  /// \return Pointer to newly created or existing element
  template <class... Args>
  Elem* emplace(Args&&... args) {
    auto found = entries_.find(Key(std::forward<Args>(args)...));
    if (found == entries_.end()) {
      // Can't use make_unique because Elem constructor is not public
      std::unique_ptr<Elem> entry(new Elem(entries_.size(), std::forward<Args>(args)...));
      found = entries_.emplace(entry->key(), std::move(entry)).first;
    }
    return found->second.get();
  }

  size_type size() const { return entries_.size(); }
  const_iterator begin() const { return entries_.begin(); }
  const_iterator end() const { return entries_.end(); }

 private:
  TableType entries_;
};

// Entry types
// These are created by the SymbolTable, not by the user

/// Table entry with index (to faciliate code generation)
/// \tparam Key Key type for SymbolTable
/// \tparam Elem Element type
template <class Key, class Elem>
class IndexedEntry {
 public:
  typedef std::size_t IdType;

  IdType id() const { return id_; }
  const Elem& value() const { return value_; }

  friend std::ostream& operator<<(std::ostream& os, const IndexedEntry& s) {
    return os << s.value_;
  }

  friend std::ostream& operator<<(std::ostream& os, const IndexedEntry* s) {
    return os << s->value_;
  }

 protected:
  typedef Key KeyType;

  IdType id_;
  const Elem value_;

  template <class... Args>
  IndexedEntry(size_t id, Args&&... args)
      : id_(id), value_(std::forward<Args>(args)...) {}

  const Key key() const { return Key(value_); }
};

class StringEntry : public IndexedEntry<StringRef, std::string> {
 private:
  StringEntry(IdType id, const std::string& value) : IndexedEntry(id, value) {}
  StringEntry(IdType id, const char* string, std::size_t length)
      : IndexedEntry(id, string, string + length) {}

  friend class SymbolTable<StringEntry>;
};

class Int32Entry : public IndexedEntry<int32_t, int32_t> {
 private:
  Int32Entry(IdType id, int32_t value) : IndexedEntry(id, value) {}

  friend class SymbolTable<Int32Entry>;
};

// Symbols are StringEntry's in SymbolTable
typedef StringEntry Symbol;

// Global table of identifiers
extern SymbolTable<Symbol>& gIdentTable;

// Global table of strings (in string literals)
extern SymbolTable<StringEntry>& gStringTable;

// Global table of integers (in integer literals)
extern SymbolTable<Int32Entry>& gIntTable;

/// Initializer type for controlling static initialization of the SymbolTables using
/// the "Nifty Counter Idiom"
static struct SymbolTablesInitializer {
  SymbolTablesInitializer ();
  ~SymbolTablesInitializer ();
} gSymbolTablesInitializer;

} // namespace cool

namespace std {

template <>
struct hash<cool::StringRef> {
  std::size_t operator()(const cool::StringRef& s) const {
    // https://stackoverflow.com/questions/35985960/c-why-is-boosthash-combine-the-best-way-to-combine-hash-values
    std::size_t seed = 0;
    std::hash<char> hasher;
    for (const char* it = s.data(), * end = it + s.size(); it != end; ++it) {
      seed ^= hasher(*it) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};
}