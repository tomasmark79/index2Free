// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#include "../src/AppCore.hpp"
#include <gtest/gtest.h>

TEST (indexLogic, HandlesArguments) {
  const char* argv[] = { "index", "--help" };
  EXPECT_EQ (runindex (2, argv), 0);
}

TEST (indexLogic, HandlesArgumentsNoLibrary) {
  const char* argv[] = { "index", "--omit" };
  EXPECT_EQ (runindex (2, argv), 0);
}

TEST (indexLogic, HandlesArgumentsLog2File) {
  const char* argv[] = { "index", "--log2file" };
  EXPECT_EQ (runindex (2, argv), 0);
}
