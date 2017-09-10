#include <gtest/gtest.h>
#include "stringtab.h"

TEST(StringTableTest, MaintainsUniqueIdentifiers) {
  using cool::gIdentTable;
  cool::Symbol* sym = gIdentTable.emplace("Object");
  ASSERT_NE(nullptr, sym);
  EXPECT_EQ(sym, gIdentTable.emplace("Object"));
  EXPECT_EQ(0, sym->id());

  cool::Symbol* sym2 = gIdentTable.emplace("Int");
  ASSERT_NE(nullptr, sym);
  EXPECT_NE(sym, sym2);
  EXPECT_EQ(1, sym2->id());

  std::string str("Int");
  EXPECT_EQ(sym2, gIdentTable.emplace(str));
}

TEST(StringTableTest, DifferentHasMethods) {
  using cool::gIdentTable;
  cool::Symbol* sym = gIdentTable.emplace("Object");
  EXPECT_TRUE(gIdentTable.has(sym));
  EXPECT_TRUE(gIdentTable.has("Object"));
}

TEST(StringTableTest, LookupFindsElement) {
  using cool::gIdentTable;
  cool::Symbol* sym = gIdentTable.emplace("Object");
  EXPECT_EQ(sym, gIdentTable.lookup("Object"));
  EXPECT_EQ(nullptr, gIdentTable.lookup("Junk"));
}
