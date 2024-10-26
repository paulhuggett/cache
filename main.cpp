// Standard library
#include <cassert>
#include <utility>

// Standard library
#include <iomanip>
#include <iostream>
#include <string>

#include "cache.hpp"
#include "iumap.hpp"
#include "lru_list.hpp"

namespace {

int main() {
#if 0
  using kvp = std::pair<int, std::string>;
  auto const test = std::array{
      kvp{1, "one"},     kvp{2, "two"},  kvp{1, "one one"}, kvp{3, "three"},   kvp{4, "four"},
      kvp{1, "one one"}, kvp{4, "four"}, kvp{5, "five"},    kvp{1, "one one"}, kvp{6, "six"},
      kvp{6, "six"},     kvp{6, "six"},  kvp{1, "one one"},
  };
  cache<int, std::string, 4> c;
  for (auto const &v : test) {
    std::cout << v.first << ' ' << v.second << ' ';
    if (c.set(v.first, v.second)) {
      std::cout << "CACHED";
    }
    std::cout << '\n';
  }

  c.dump(std::cout);
#else
  cache<int, std::pair<int, int>, 32> c;

  // An infinite loop for use with a profiler.
  for (int ctr = 0; true; ++ctr) {
    auto c2 = ctr / 16;
    c.set(c2, std::make_pair(c2, c2));
  }

#endif
}
