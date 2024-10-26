// DUT
#include "iumap.hpp"

// Standard library
#include <string>

// Google Test/Mock
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace std::string_literals;

namespace {

TEST(IUMap, Empty) {
  iumap<int, std::string, 8> h;
  EXPECT_EQ(h.size(), 0U);
  EXPECT_TRUE(h.empty());
}

TEST(IUMap, Insert) {
  iumap<int, std::string, 8> h;
  using value_type = decltype(h)::value_type;

  auto [pos1, did_insert1] = h.insert(std::make_pair(1, "one"s));
  ASSERT_TRUE(did_insert1);
  EXPECT_FALSE(h.empty());
  EXPECT_EQ(h.size(), 1U);
  EXPECT_EQ(*pos1, (value_type{1, "one"s}));

  auto [pos2, did_insert2] = h.insert(std::make_pair(2, "two"s));
  ASSERT_TRUE(did_insert2);
  EXPECT_FALSE(h.empty());
  EXPECT_EQ(h.size(), 2U);
  EXPECT_EQ(*pos2, (value_type{2, "two"s}));

  auto [pos3, did_insert3] = h.insert(std::make_pair(3, "three"s));
  ASSERT_TRUE(did_insert3);
  EXPECT_FALSE(h.empty());
  EXPECT_EQ(h.size(), 3U);
  EXPECT_EQ(*pos3, (value_type{3, "three"s}));
}

TEST(IUMap, InsertOrAssign) {
  iumap<int, std::string, 8> h;
  using value_type = decltype(h)::value_type;

  auto [pos1, did_insert1] = h.insert_or_assign(10, "ten"s);
  ASSERT_TRUE(did_insert1);
  EXPECT_EQ(h.size(), 1U);
  EXPECT_EQ(*pos1, (value_type{10, "ten"s}));

  auto [pos2, did_insert2] = h.insert_or_assign(10, "ten ten"s);
  EXPECT_FALSE(did_insert2);
  EXPECT_EQ(h.size(), 1U);
  EXPECT_EQ(*pos2, (value_type{10, "ten ten"s}));
}

TEST(IUMap, Erase) {
  iumap<int, std::string, 8> h;
  auto [pos1, did_insert1] = h.insert(std::make_pair(10, "ten"s));
  h.erase(pos1);
  EXPECT_EQ(h.size(), 0U);
  EXPECT_TRUE(h.empty());
}

}  // end anonymous namespace
