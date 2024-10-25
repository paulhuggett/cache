//
//  cache.hpp
//  hasht
//
//  Created by Paul Bowen-Huggett on 24/10/2024.
//

#ifndef CACHE_HPP
#define CACHE_HPP

#include <cassert>
#include <cstddef>
#include <ostream>
#include <utility>

#include "iumap.hpp"
#include "lru_list.hpp"

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
  auto pos = h.find(k);
  if (pos == h.end()) {
    auto const fn = [this](std::pair<key_type, value_type> & kvp) {
      // delete the key being evicted from the LUR-list from the hash table so that
      // they always match.
      if (auto const evict_pos = this->h.find(kvp.first); evict_pos != this->h.end()) {
        this->h.erase(evict_pos);
      }
    };
    h.insert(std::make_pair(k, &lru.add(std::make_pair(k, v), fn)));
    assert(lru.size() == h.size());
    return false;
  }

  // The key _was_ found in the cache.
  typename lru_container::node &lru_entry = *pos->second;
  lru.touch(lru_entry);
  auto &cached_value = static_cast<std::pair<Key, Value> &>(lru_entry).second;
  if (cached_value == v) {
    // The key was in the cache and the values are equal.
    assert(lru.size() == h.size());
    return true;
  }
  cached_value = v;
  assert(lru.size() == h.size());
  return false;
}

#endif  // CACHE_HPP
