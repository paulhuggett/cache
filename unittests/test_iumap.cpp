// DUT
#include "iumap.hpp"

// Standard library
#include <string>
#include <unordered_map>

// Google Test/Mock
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#if defined(FUZZTEST) && FUZZTEST
#include <fuzztest/fuzztest.h>
#endif

using namespace std::string_literals;

namespace {

TEST(IUMap, Empty) {
  iumap<int, std::string, 8> h;
  EXPECT_EQ(h.size(), 0U);
  EXPECT_EQ(h.max_size(), 8U);
  EXPECT_EQ(h.capacity(), 8U);
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

TEST(IUMap, InsertIntoAFullMap) {
  iumap<int, std::string, 2> h;
  h.insert(std::make_pair(1, "one"s));
  h.insert(std::make_pair(2, "two"s));
  auto [pos, did_insert] = h.insert(std::make_pair(3, "three"s));
  ASSERT_FALSE(did_insert);
  EXPECT_EQ(pos, h.end());
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

TEST(IUMap, InsertOrAssignIntoAFullMap) {
  iumap<int, std::string, 2> h;
  h.insert(std::make_pair(1, "one"s));
  h.insert(std::make_pair(2, "two"s));
  auto [pos, did_insert] = h.insert_or_assign(3, "three"s);
  ASSERT_FALSE(did_insert);
  EXPECT_EQ(pos, h.end());
}

TEST(IUMap, Erase) {
  iumap<int, std::string, 8> h;
  auto [pos1, did_insert1] = h.insert(std::make_pair(10, "ten"s));
  h.erase(pos1);
  EXPECT_EQ(h.size(), 0U);
  EXPECT_TRUE(h.empty());
}

TEST(IUMap, FindFound) {
  iumap<int, std::string, 8> h;
  h.insert(std::make_pair(10, "ten"s));
  auto pos = h.find(10);
  ASSERT_NE(pos, h.end());
  EXPECT_EQ(pos->first, 10);
  EXPECT_EQ(pos->second, "ten"s);
}

TEST(IUMap, FindNotFound) {
  iumap<int, std::string, 8> h;
  h.insert(std::make_pair(10, "ten"s));
  auto pos = h.find(11);
  EXPECT_EQ(pos, h.end());
}

TEST(IUMap, CopyAssign) {
  iumap<int, std::string, 4> a;
  a.insert(std::make_pair(1, "one"));
  auto pa2 = a.insert(std::make_pair(2, "two")).first;
  a.insert(std::make_pair(3, "three"));
  a.erase(pa2);  // an erase so that the container holds a tombstone record

  auto const four = std::pair<int const, std::string>{4, "four"s};
  auto const six = std::pair<int const, std::string>{6, "six"s};
  auto const seven = std::pair<int const, std::string>{7, "seven"s};

  iumap<int, std::string, 4> b;
  b.insert(four);
  auto pb5 = b.insert(std::make_pair(5, "five")).first;
  b.insert(six);
  b.erase(pb5);  // an erase so that the container holds a tombstone record
  b.insert(seven);

  a = b;
  EXPECT_EQ(a.size(), 3);
  EXPECT_EQ(a.find(1), a.end());
  EXPECT_EQ(a.find(2), a.end());
  EXPECT_EQ(a.find(3), a.end());

  ASSERT_NE(a.find(4), a.end());
  EXPECT_EQ(*a.find(4), four);

  EXPECT_EQ(a.find(5), a.end());

  ASSERT_NE(a.find(6), a.end());
  EXPECT_EQ(*a.find(6), six);
  ASSERT_NE(a.find(7), a.end());
  EXPECT_EQ(*a.find(7), seven);
}

TEST(IUMap, MoveAssign) {
  iumap<int, std::string, 4> a;
  a.insert(std::make_pair(1, "one"));
  auto pa2 = a.insert(std::make_pair(2, "two")).first;
  a.insert(std::make_pair(3, "three"));
  a.erase(pa2);  // an erase so that the container holds a tombstone record

  auto const four = std::pair<int const, std::string>{4, "four"s};
  auto const six = std::pair<int const, std::string>{6, "six"s};
  auto const seven = std::pair<int const, std::string>{7, "seven"s};

  iumap<int, std::string, 4> b;
  b.insert(four);
  auto pb5 = b.insert(std::make_pair(5, "five")).first;
  b.insert(six);
  b.erase(pb5);  // an erase so that the container holds a tombstone record
  b.insert(seven);

  a = std::move(b);
  EXPECT_EQ(a.size(), 3);
  EXPECT_EQ(a.find(1), a.end());
  EXPECT_EQ(a.find(2), a.end());
  EXPECT_EQ(a.find(3), a.end());

  ASSERT_NE(a.find(4), a.end());
  EXPECT_EQ(*a.find(4), four);

  EXPECT_EQ(a.find(5), a.end());

  ASSERT_NE(a.find(6), a.end());
  EXPECT_EQ(*a.find(6), six);
  ASSERT_NE(a.find(7), a.end());
  EXPECT_EQ(*a.find(7), seven);
}

TEST(IUMap, CopyCtor) {
  auto const one = std::pair<int const, std::string>{1, "one"s};
  auto const three = std::pair<int const, std::string>{3, "three"s};

  iumap<int, std::string, 4> a;
  a.insert(one);
  auto pa2 = a.insert(std::make_pair(2, "two")).first;
  a.insert(three);
  a.erase(pa2);  // an erase so that the container holds a tombstone record

  iumap<int, std::string, 4> b(a);
  EXPECT_EQ(b.size(), 2U);
  ASSERT_NE(b.find(1), b.end());
  EXPECT_EQ(*a.find(1), one);
  ASSERT_EQ(a.find(2), a.end());
  ASSERT_NE(a.find(3), a.end());
  EXPECT_EQ(*a.find(3), three);
}

TEST(IUMap, MoveCtor) {
  auto const one = std::pair<int const, std::string>{1, "one"s};
  auto const three = std::pair<int const, std::string>{3, "three"s};

  iumap<int, std::string, 4> a;
  a.insert(one);
  auto pa2 = a.insert(std::make_pair(2, "two")).first;
  a.insert(three);
  a.erase(pa2);  // an erase so that the container holds a tombstone record

  iumap<int, std::string, 4> const b{std::move(a)};
  EXPECT_EQ(b.size(), 2U);
  ASSERT_NE(b.find(1), b.end());
  EXPECT_EQ(*b.find(1), one);
  ASSERT_EQ(b.find(2), b.end());
  ASSERT_NE(b.find(3), b.end());
  EXPECT_EQ(*b.find(3), three);
}

TEST(IUMap, IteratorAdd) {
  iumap<int, int, 4> a;
  a.try_emplace(1, 1);
  a.try_emplace(2, 2);
  a.try_emplace(3, 3);
  auto pos = a.begin();
  pos++;
  pos += 1;
  pos += -2;
  pos = pos + 2;
  pos = pos - 2;
  EXPECT_EQ(pos, a.begin());
}

TEST(IUMap, IteratorSubtract) {
  iumap<int, int, 4> a;
  a.try_emplace(1, 1);
  a.try_emplace(2, 2);
  a.try_emplace(3, 3);
  auto pos = a.end();
  pos--;
  pos -= 1;
  pos -= -2;
  pos = pos - 2;
  pos = pos + 2;
  EXPECT_EQ(pos, a.end());
}

void Thrash(std::vector<int> const &in, std::vector<int> const &del) {
  iumap<int, int, 16> a;
  std::unordered_map<int, int> b;

  for (int const a1 : in) {
    if (a.size() >= a.max_size()) {
      break;
    }
    auto const value = std::make_pair(a1, a1);
    a.insert(value);
    b.insert(value);
  }
  for (int const d1 : del) {
    if (auto const apos = a.find(d1); apos != a.end()) {
      a.erase(apos);
    }
    if (auto const bpos = b.find(d1); bpos != b.end()) {
      b.erase(bpos);
    }
  }
  EXPECT_THAT(a, testing::UnorderedElementsAreArray(b));
}

#if defined(FUZZTEST) && FUZZTEST
FUZZ_TEST(IUMap, Thrash);
#endif
TEST(IUMap, ThreashNone) {
  Thrash(std::vector<int>{}, std::vector<int>{});
}

}  // end anonymous namespace
