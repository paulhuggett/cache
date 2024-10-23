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

// Standard library: for quick and dirty trace output
#include <iomanip>
#include <iostream>

template <typename Key, typename Mapped, std::size_t Size, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
class iumap {
  struct member;

public:
  using key_type = Key;
  using mapped_type = Mapped;
  using value_type = std::pair<Key const, Mapped>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  using hasher = Hash;
  using key_equal = KeyEqual;

  using reference = value_type&;
  using const_reference = value_type const&;
  using pointer = value_type*;
  using const_pointer = value_type const*;

  constexpr iumap() noexcept;
  ~iumap() noexcept;

  // Capacity
  constexpr auto max_size() const noexcept { return v_.max_size(); }

  // Modifiers
  void clear() noexcept;
  template <typename... Args> void try_emplace(Key const& key, Args&&... args);
  /// inserts an element or assigns to the current element if the key already exists
  template <typename M> void insert_or_assign(Key const& key, M&& value);
  /// inserts in-place if the key does not exist, does nothing if the key exists
  template <typename... Args> void emplace(Key const& key, Args&&... args);
  // Lookup
  value_type* find(Key const& k) const;
  // Mapped& operator[](Key const &key);

  // Observers
  hasher hash_function() const { return Hash{}; }
  key_equal key_eq() const { return KeyEqual{}; }

  void dump(std::ostream& os) const {
    for (auto index = std::size_t{0}; auto const& slot : v_) {
      os << '[' << index << "] ";
      ++index;
      if (slot.occupied) {
        auto const* const kvp = slot.cast();
        os << "> " << kvp->first << '=' << std::quoted(kvp->second);
      } else {
        os << '*';
      }
      os << '\n';
    }
  }

private:
  struct member {
    [[nodiscard]] constexpr value_type* cast() noexcept {
      assert(occupied);
      return std::bit_cast<value_type*>(&kvp[0]);
    };
    [[nodiscard]] constexpr value_type const* cast() const noexcept {
      assert(occupied);
      return std::bit_cast<value_type*>(&kvp[0]);
    };

    member* next = nullptr;
    ;
    member* prev = nullptr;
    bool occupied = false;
    alignas(value_type) std::byte kvp[sizeof(value_type)]{};
  };
  member* first_ = nullptr;
  member* last_ = nullptr;

  std::array<member, Size> v_{};

  template <typename Container> static auto* lookup_slot(Container& container, Key const& key);
};

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
constexpr iumap<Key, Mapped, Size, Hash, KeyEqual>::iumap() noexcept {
  auto* const limit = &v_[Size - 2];
  auto* curr = &v_[0];
  auto* next = &v_[1];
  member* prev = nullptr;
  first_ = curr;
  while (curr != limit) {
    curr->next = curr + 1;
    curr->prev = prev;
    prev = curr;
    curr = next;
    ++next;
  }
  last_ = prev;
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
iumap<Key, Mapped, Size, Hash, KeyEqual>::~iumap() noexcept {
  this->clear();
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
void iumap<Key, Mapped, Size, Hash, KeyEqual>::clear() noexcept {
  for (auto& entry : v_) {
    if (entry.occupied) {
      entry.cast()->~value_type();
      entry.occupied = false;
    }
  }
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
template <typename... Args>
void iumap<Key, Mapped, Size, Hash, KeyEqual>::try_emplace(Key const& key, Args&&... args) {
  auto* const slot = iumap::lookup_slot(*this, key);
  if (slot == nullptr) {
    // The map is full and the key was not found. Insertion failed.
    return std::make_pair(this->end(), false);
  }
  auto const it = iterator{slot, &v_};
  auto const do_insert = slot->state == state::unused;
  if (do_insert) {
    // Not found. Add a new key/value pair.
    new (slot->kvp) value_type(key, std::forward<Args>(args)...);
    slot->state = state::occupied;
    ++size_;
  }
  return std::make_pair(it, do_insert);
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
void lru<Key, Mapped, Size, Hash, KeyEqual>::insert(value_type const& value) {
  return try_emplace(value.first, value.second);
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
template <typename M>
auto iumap<Key, Mapped, Size, Hash, KeyEqual>::insert_or_assign(Key const& key, M&& value)
    -> std::pair<iterator, bool> {
  auto* const slot = iumap::lookup_slot(*this, key);
  if (slot == nullptr) {
    // The map is full and the key was not found. Insertion failed.
    return std::make_pair(this->end(), false);
  }
  if (slot->state == state::unused) {
    // Not found. Add a new key/value pair.
    new (slot->kvp) value_type(key, std::forward<M>(value));
    slot->state = state::occupied;
    ++size_;
    return std::make_pair(iterator{slot, &v_}, true);
  }
  auto* const kvp = slot->cast();
  kvp->second = value;  // Overwrite the existing value.
  return std::make_pair(iterator{slot, &v_}, false);
}

#if 0
template<typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
Mapped& iumap<Key, Mapped, Size, Hash, KeyEqual>::operator[](Key const &key) {
  auto * const slot = iumap::lookup_slot(*this, key);
  assert (slot != nullptr && "The map is full and the key was not found.");
  if (!slot->occupied) {
    // Not found. Add a new key/value pair.
    new (slot->kvp) value_type(key, Mapped{});
    slot->occupied = true;
    ++size_;
  }
  return slot->cast()->second;
}
#endif

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
auto iumap<Key, Mapped, Size, Hash, KeyEqual>::find(Key const& k) const -> const_iterator {
  auto* const slot = iumap::lookup_slot(*this, k);
  if (slot == nullptr || slot->state != state::occupied) {
    return this->end();  // Not found
  }
  return {slot, &v_};  // Found
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
template <typename Container>
auto* iumap<Key, Mapped, Size, Hash, KeyEqual>::lookup_slot(Container& container, Key const& key) {
  auto& v = container.v_;
  auto const size = v.size();
  auto pos = Hash{}(key) % size;
  for (auto iteration = 1U; iteration <= size; ++iteration) {
    auto& slot = v[pos];
    if (slot.state == state::unused || KeyEqual{}(slot.cast()->first, key)) {
      return &slot;
    }
    pos = (pos + iteration) % size;
  }
  return static_cast<decltype(v.data())>(nullptr);  // No available slot.
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual> class cache {
public:
  struct lru {
    lru* next = nullptr;
    lru* prev = nullptr;
    Mapped value;
  };
  using map_type = iumap<Key, lru, Size, Hash, KeyEqual>;

  using iterator = map_type::iterator;
  using const_iterator = map_type::const_iterator;

  lru* first = nullptr;
  lru* last = nullptr;

  iumap<Key, lru, Size, Hash, KeyEqual> h;

  // Modifiers
  void clear() noexcept { h.clear(); }
  /// inserts elements
  std::pair<iterator, bool> insert(Mapped const& value) { auto h.insert(value); }
  /// inserts an element or assigns to the current element if the key already exists
  template <typename M> std::pair<iterator, bool> insert_or_assign(Key const& key, M&& value);
  /// inserts in-place if the key does not exist, does nothing if the key exists
  template <typename... Args> std::pair<iterator, bool> try_emplace(Key const& key, Args&&... args);
  // Lookup
  const_iterator find(Key const& k) const;
  // Mapped& operator[](Key const &key);

  std::string lookup(int k, std::string const& v) { auto [pos, did_insert] = h.insert_or_assign(k, v); }
};

#include <string>

int main() {
  iumap<int, std::string, 16> h;

  h.insert(std::make_pair(2, "two"));
  h.insert(std::make_pair(18, std::to_string(18)));
  h.insert(std::make_pair(19, std::to_string(19)));

  auto& os = std::cout;
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
