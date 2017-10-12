//
// Created by Linderman, Michael D. on 8/16/17.
//
#include <gtest/gtest.h>
#include "stringtab.h"
#include "scopedtab.h"

TEST(SymTabTest, ReturnsMostCloselyNestedScope) {
  // Create string table and test keys (we don't use gIdentTable to prevent crosstalk with
  // other tests
  cool::SymbolTable<cool::Symbol> string_table;
  cool::Symbol *foo = string_table.emplace("foo"), *bar = string_table.emplace("bar"), *baz = string_table.emplace("baz");

  // Possible values
  int a=1, b=2, c=3;

  cool::ScopedTable<cool::Symbol*,int> table;

  ASSERT_EQ(nullptr, table.Lookup(foo));

  EXPECT_EQ(&a, table.AddToScope(foo, &a));
  EXPECT_EQ(&b, table.AddToScope(bar, &b));

  table.EnterScope();

  EXPECT_EQ(&c, table.AddToScope(foo, &c));
  EXPECT_EQ(&c, table.Lookup(foo));

  EXPECT_EQ(&b, table.Lookup(bar));
  EXPECT_EQ(nullptr, table.Probe(bar));

  EXPECT_EQ(&a, table.AddToScope(baz, &a));

  table.ExitScope();

  EXPECT_EQ(&a, table.Lookup(foo));
  EXPECT_EQ(&b, table.Lookup(bar));
  EXPECT_EQ(nullptr, table.Lookup(baz));
}
