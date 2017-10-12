#include <gtest/gtest.h>
#include "stringtab.h"

TEST(StringTableTest, MaintainsUniqueIdentifiers) {
  cool::SymbolTable<cool::Symbol> string_table;

  cool::Symbol* sym = string_table.emplace("Object");
  ASSERT_NE(nullptr, sym);
  EXPECT_EQ(sym, string_table.emplace("Object"));
  EXPECT_EQ(0UL, sym->id());

  cool::Symbol* sym2 = string_table.emplace("Int");
  ASSERT_NE(nullptr, sym);
  EXPECT_NE(sym, sym2);
  EXPECT_EQ(1UL, sym2->id());

  std::string str("Int");
  EXPECT_EQ(sym2, string_table.emplace(str));
}

TEST(StringTableTest, DifferentHasMethods) {
  cool::SymbolTable<cool::Symbol> string_table;
  cool::Symbol* sym = string_table.emplace("Object");
  EXPECT_TRUE(string_table.has(sym));
  EXPECT_TRUE(string_table.has("Object"));
}

TEST(StringTableTest, LookupFindsElement) {
  cool::SymbolTable<cool::Symbol> string_table;
  cool::Symbol* sym = string_table.emplace("Object");
  EXPECT_EQ(sym, string_table.lookup("Object"));
  EXPECT_EQ(nullptr, string_table.lookup("Junk"));
}
