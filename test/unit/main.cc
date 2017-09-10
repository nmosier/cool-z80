//
// Created by Michael Linderman on 8/13/17.
//

#include <gtest/gtest.h>
#include "cool_parse.h"

// Define globals expected by modules in the compiler
YYSTYPE cool_yylval;

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
