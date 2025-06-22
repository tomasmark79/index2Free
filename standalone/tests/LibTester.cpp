// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#include "../src/AppCore.hpp"
#include <gtest/gtest.h>

TEST (AppLogic, HandlesArguments) {
  const char* argv[] = { "index2", "--help" };
  EXPECT_EQ (runApp (2, argv), 0);
}

TEST (AppLogic, HandlesArgumentsNoLibrary) {
  const char* argv[] = { "index2", "--omit" };
  EXPECT_EQ (runApp (2, argv), 0);
}

TEST (AppLogic, HandlesArgumentsLog2File) {
  const char* argv[] = { "index2", "--log2file" };
  EXPECT_EQ (runApp (2, argv), 0);
}
