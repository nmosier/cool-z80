#include <gtest/gtest.h>
#include "ast.h"

TEST(gStringTableTest, MaintainsStringEntries) {
  using namespace cool;

  StringLiteral* lit = StringLiteral::Create("test string");
  ASSERT_NE(nullptr, lit);
  EXPECT_TRUE(gStringTable.has("test string"));
}

TEST(gIntTableTest, MaintainsIntEntries) {
  using namespace cool;

  IntLiteral* lit = IntLiteral::Create(3);
  ASSERT_NE(nullptr, lit);
  EXPECT_TRUE(gIntTable.has(3));
}
