#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "iumap.hpp"

namespace {

TEST(IUMap, Empty) {
  iumap<int, std::string, 8> h;
  EXPECT_EQ(h.size(), 0U);
  EXPECT_TRUE(h.empty());
}

TEST(IUMap, Insert) {
  iumap<int, std::string, 8> h;

  auto [pos1, did_insert1] = h.insert(std::make_pair(1, "one"));
  ASSERT_TRUE(did_insert1);
  EXPECT_FALSE(h.empty());
  EXPECT_EQ(h.size(), 1U);
  EXPECT_EQ(*pos1, std::make_pair(1, "one"));

  auto [pos2, did_insert2] = h.insert(std::make_pair(2, "two"));
  ASSERT_TRUE(did_insert2);
  EXPECT_FALSE(h.empty());
  EXPECT_EQ(h.size(), 2U);
  EXPECT_EQ(*pos2, std::make_pair(2, "two"));

  auto [pos3, did_insert3] = h.insert(std::make_pair(3, "three"));
  ASSERT_TRUE(did_insert3);
  EXPECT_FALSE(h.empty());
  EXPECT_EQ(h.size(), 3U);
  EXPECT_EQ(*pos3, std::make_pair(3, "three"));
}

TEST(IUMap, InsertOrAssign) {
  iumap<int, std::string, 8> h;
  auto [pos1, did_insert1] = h.insert_or_assign(10, "ten");
  ASSERT_TRUE(did_insert1);
  EXPECT_EQ(h.size(), 1U);
  EXPECT_EQ(*pos1, std::make_pair(10, "ten"));

  auto [pos2, did_insert2] = h.insert_or_assign(10, "ten ten");
  EXPECT_FALSE(did_insert2);
  EXPECT_EQ(h.size(), 1U);
  EXPECT_EQ(*pos2, std::make_pair(10, "ten ten"));
}

TEST(IUMap, Erase) {
  iumap<int, std::string, 8> h;
  auto [pos1, did_insert1] = h.insert(std::make_pair(10, "ten"));
  h.erase(pos1);
  EXPECT_EQ(h.size(), 0U);
  EXPECT_TRUE(h.empty());
}

}  // end anonymous namespace
