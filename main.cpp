// Standard library
#include <array>
#include <bit>
#include <cassert>
#include <compare>
#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>

// Standard library
#include <iomanip>
#include <iostream>
#include <string>

#include "cache.hpp"

void check_iumap() {
  iumap<int, std::string, 16> h;

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
  // h[101] = "one zero one";
  // h[23] = "twenty three";

  h.dump(os);
  os << "---\n";

  std::cout << "Members are:\n";
  for (auto it = h.begin(), end = h.end(); it != end; ++it) {
    std::cout << "  " << it->first << ' ' << it->second << '\n';
  }
}

void check_lru_list() {
  lru_list<int, 5> lru;
  auto *t1 = lru.add(1);
  lru.touch(t1);  // do nothing!
  auto *t2 = lru.add(2);
  lru.touch(t2);  // do nothing!
  auto *t3 = lru.add(3);
  auto *t4 = lru.add(4);
  lru.touch(t1);
  lru.dump(std::cout);

  auto *t5 = lru.add(5);  // evicts 2
  auto *t6 = lru.add(6);  // evicts 3
  lru.dump(std::cout);

  lru.touch(t3);  // 3 is now at the front of the list
  lru.dump(std::cout);
  auto *t7 = lru.add(7);  // evicts 4
  lru.dump(std::cout);
}

#if 1
namespace std {

ostream &operator<<(ostream &os, pair<int, string> const &kvp) {
  return os << '{' << kvp.first << ':' << kvp.second << '}';
}

}  // end namespace std
#endif

template <typename Key, typename Value, std::size_t Size> class cache {
public:
  using key_type = Key;
  using value_type = Value;

  value_type *find(key_type const &k);
  bool set(key_type const &k, value_type const &v);

  void dump(std::ostream &os) {
    lru.dump(os);
    h.dump(os);
  }

private:
  using lru_container = lru_list<std::pair<Key, Value>, Size>;
  using hash_map = iumap<Key const, typename lru_container::node *, Size>;

  lru_container lru;
  hash_map h;
};

template <typename Key, typename Value, std::size_t Size>
auto cache<Key, Value, Size>::find(key_type const &k) -> value_type * {
  auto const pos = h.find(k);
  if (pos == h.end()) {
    return nullptr;
  }
  auto *const node = pos->second;
  lru.touch(node);
  assert(lru.size() == h.size());
  return &(node->value()->second);
}

template <typename Key, typename Value, std::size_t Size>
bool cache<Key, Value, Size>::set(key_type const &k, value_type const &v) {
  auto const pos = h.find(k);
  if (pos == h.end()) {
    auto const fn = [this](std::pair<key_type, value_type> *const kvp) {
      // delete the key being evicted
      if (auto const pos = this->h.find(kvp->first); pos != this->h.end()) {
        this->h.erase(pos);
      }
    };
    h.insert(std::make_pair(k, lru.add(std::make_pair(k, v), fn)));
    assert(lru.size() == h.size());
    return false;
  }

  // The key _was_ found in the cache.
  lru.touch(pos->second);
  auto &cached_value = pos->second->value()->second;
  if (v == cached_value) {
    assert(lru.size() == h.size());
    return true;
  }
  cached_value = v;
  assert(lru.size() == h.size());
  return false;
}

int main() {
#if 1
  check_iumap();
  check_lru_list();
  std::cout << "---\n";
#endif

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
}
