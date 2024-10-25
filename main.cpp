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

void check_iumap() {
  iumap<int, std::string, 8> h;

  h.insert(std::make_pair(2, "two"));
  h.insert(std::make_pair(18, std::to_string(18)));
  h.insert(std::make_pair(19, std::to_string(19)));

  auto &os = std::cout;
  h.dump(os);
  os << "---\n";

  for (auto const k : {1, 2, 3, 18, 19, 20}) {
    os << k << ' ';
    if (auto pos = h.find(k); pos != h.end()) {
      os << std::quoted(pos->second);
    } else {
      os << "not found";
    }
    os << '\n';
  }
  os << "---\n";

  auto [insert_pos, did_insert] = h.insert_or_assign(19, std::to_string(20));
  os << insert_pos->first << ' ' << std::quoted(insert_pos->second) << '\n';

  auto make_kvp = [](int k) { return std::make_pair(k, std::to_string(k)); };

  h.insert(make_kvp(21));
  h.insert(make_kvp(22));
  h.insert(make_kvp(23));
  h.insert(make_kvp(14));
  h.insert(make_kvp(9));

  h.try_emplace(101, "one zero one");
  h.try_emplace(23, "twenty three");

  h.dump(os);
  os << "---\n";

  std::cout << "Members are:\n";
  for (auto const &[key, value] : h) {
    std::cout << "  " << key << ' ' << value << '\n';
  }

  h.clear();

  while (h.size() < h.max_size()) {
    auto const ctr = static_cast<int>(h.size());
    h.insert(std::make_pair(ctr, std::to_string(ctr)));
  }

  while (!h.empty()) {
    auto const ctr = static_cast<int>(h.size() - 1);
    h.erase(h.find(ctr));
  }
  h.insert(make_kvp(9));
}

}  // end anonymous namespace

int main() {
  check_iumap();
  std::cout << "---\n";

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
