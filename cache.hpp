//
//  cache.hpp
//  hasht
//
//  Created by Paul Bowen-Huggett on 23/10/2024.
//

#ifndef CACHE_HPP
#define CACHE_HPP

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

// An in-place unordered hash table.
template <typename Key, typename Mapped, std::size_t Size, typename Hash = std::hash<std::remove_cv_t<Key>>,
          typename KeyEqual = std::equal_to<Key>>
class iumap {
  friend class iterator;
  struct member;

public:
  using key_type = Key;
  using mapped_type = Mapped;
  using value_type = std::pair<Key const, Mapped>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  using hasher = Hash;
  using key_equal = KeyEqual;

  using reference = value_type &;
  using const_reference = value_type const &;
  using pointer = value_type *;
  using const_pointer = value_type const *;

  template <typename T> class sentinel {};

  template <typename T>
    requires(std::is_same_v<T, member> || std::is_same_v<T, member const>)
  class iterator_type {
    friend iterator_type<std::remove_const_t<T>>;
    friend iterator_type<T const>;

  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = iumap::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = std::conditional_t<std::is_const_v<T>, value_type const *, value_type *>;
    using reference = std::conditional_t<std::is_const_v<T>, value_type const &, value_type &>;

    using container_type =
        std::conditional_t<std::is_const_v<T>, std::array<member, Size> const, std::array<member, Size>>;

    constexpr iterator_type(T *const slot, container_type *const container) noexcept
        : slot_{slot}, container_{container} {
      this->move_forward_to_occupied();
    }

    constexpr bool operator==(iterator_type<std::remove_const_t<T>> const &other) const noexcept {
      return slot_ == other.slot_ && container_ == other.container_;
    }
    constexpr bool operator==(iterator_type<T const> const &other) const noexcept {
      return slot_ == other.slot_ && container_ == other.container_;
    }
    constexpr bool operator!=(iterator_type<std::remove_const_t<T>> const &other) const noexcept {
      return !operator==(other);
    }
    constexpr bool operator!=(iterator_type<T const> const &other) const noexcept { return !operator==(other); }

    constexpr bool operator<=>(iterator_type<T const> const &other) const noexcept {
      return std::make_pair(slot_, container_) <=> std::make_pair(other.slot_, other.container_);
    }

    constexpr auto operator==(sentinel<T>) const noexcept { return slot_ == this->end_limit(); }
    constexpr auto operator!=(sentinel<T> other) const noexcept { return !operator==(other); }

    reference operator*() const noexcept { return *slot_->cast(); }
    pointer operator->() const noexcept { return slot_->cast(); }

    iterator_type &operator--() {
      --slot_;
      this->move_backward_to_occupied();
      return *this;
    }
    iterator_type &operator++() {
      ++slot_;
      this->move_forward_to_occupied();
      return *this;
    }
    iterator_type operator++(int) {
      auto const tmp = *this;
      ++*this;
      return tmp;
    }
    iterator_type operator--(int) {
      auto const tmp = *this;
      --*this;
      return tmp;
    }

    iterator_type &operator+=(difference_type n) {
      if (n < 0) {
        (*this) = (*this) - (-n);
      } else {
        for (; n > 0; --n) {
          ++(*this);
        }
      }
      return *this;
    }

    iterator_type &operator-=(difference_type n) {
      if (n < 0) {
        (*this) = (*this) + (-n);
      } else {
        for (; n > 0; --n) {
          --(*this);
        }
      }
      return *this;
    }

    iterator_type operator+(difference_type n) const {
      auto result = *this;
      result += n;
      return result;
    }
    iterator_type operator-(difference_type n) const {
      auto result = *this;
      result -= n;
      return result;
    }

    T *raw() { return slot_; }

  private:
    constexpr auto end_limit() { return container_->data() + container_->size(); }
    constexpr void move_backward_to_occupied() {
      auto const *const limit = container_->data();
      while (slot_ >= limit && slot_->state != state::occupied) {
        --slot_;
      }
    }
    constexpr void move_forward_to_occupied() {
      auto const *const end = this->end_limit();
      while (slot_ < end && slot_->state != state::occupied) {
        ++slot_;
      }
    }

    T *slot_;
    container_type *container_;
  };

  using iterator = iterator_type<member>;
  using const_iterator = iterator_type<member const>;

  constexpr iumap() noexcept = default;
  ~iumap() noexcept;

  // Iterators
  constexpr auto begin() { return iterator{v_.data(), &v_}; }
  constexpr auto begin() const { return const_iterator{v_.data(), &v_}; }

  constexpr auto end() noexcept { return iterator{v_.data() + v_.size(), &v_}; }
  constexpr auto end() const noexcept { return const_iterator{v_.data() + v_.size(), &v_}; }

  // Capacity
  constexpr auto empty() const noexcept { return size_ == 0; }
  constexpr auto size() const noexcept { return size_; }
  constexpr auto max_size() const noexcept { return v_.max_size(); }
  constexpr auto capacity() const noexcept { return Size; }

  // Modifiers
  void clear() noexcept;
  /// inserts elements
  std::pair<iterator, bool> insert(value_type const &value);
  /// inserts an element or assigns to the current element if the key already exists
  template <typename M> std::pair<iterator, bool> insert_or_assign(Key const &key, M &&value);
  /// inserts in-place if the key does not exist, does nothing if the key exists
  template <typename... Args> std::pair<iterator, bool> try_emplace(Key const &key, Args &&...args);
  // Lookup
  iterator find(Key const &k);
  const_iterator find(Key const &k) const;
  // Mapped& operator[](Key const &key);

  iterator erase(iterator pos) {
    member *const slot = pos.raw();
    auto const result = pos + 1;
    if (slot->state == state::occupied) {
      assert(size_ > 0);
      slot->cast()->~value_type();
      slot->state = state::tombstone;
      --size_;
    }
    return result;
  }
  iterator erase(const_iterator pos) {
    member const *const slot = pos.raw();
    auto const result = pos + 1;
    if (slot->state == state::occupied) {
      assert(size_ > 0);
      slot->cast()->~value_type();
      slot->state = state::tombstone;
      --size_;
    }
    return result;
  }
#if 0
iterator erase(const_iterator first, const_iterator last) {
}
size_type erase(Key const & key) {
}
#endif

  // Observers
  hasher hash_function() const { return Hash{}; }
  key_equal key_eq() const { return KeyEqual{}; }

  void dump(std::ostream &os) const {
    std::cout << "size=" << size_ << '\n';

    for (auto index = std::size_t{0}; auto const &slot : v_) {
      os << '[' << index << "] ";
      ++index;
      switch (slot.state) {
      case state::unused: os << '*'; break;
      case state::tombstone: os << "\xF0\x9F\xAA\xA6"; break;  // UTF-8 U+1fAA6 tomestone
      case state::occupied: {
        auto const *const kvp = slot.cast();
        os << "> " << kvp->first << '=' << kvp->second;
      } break;
      }
      os << '\n';
    }
  }

private:
  enum class state { occupied, tombstone, unused };
  struct member {
    [[nodiscard]] constexpr value_type *cast() noexcept {
      assert(state == state::occupied);
      return std::bit_cast<value_type *>(&kvp[0]);
    };
    [[nodiscard]] constexpr value_type const *cast() const noexcept {
      assert(state == state::occupied);
      return std::bit_cast<value_type const *>(&kvp[0]);
    };

    enum state state = state::unused;
    alignas(value_type) std::byte kvp[sizeof(value_type)]{};
  };
  std::size_t size_ = 0;
  std::array<member, Size> v_{};

  template <typename Container> static auto *lookup_slot(Container &container, Key const &key);

  template <typename Container> static auto *find_insert_slot(Container &container, Key const &key);
};

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
iumap<Key, Mapped, Size, Hash, KeyEqual>::~iumap() noexcept {
  this->clear();
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
void iumap<Key, Mapped, Size, Hash, KeyEqual>::clear() noexcept {
  if (this->empty()) {
    return;
  }
  for (auto &entry : v_) {
    if (entry.state == state::occupied) {
      entry.cast()->~value_type();
      entry.state = state::unused;
    }
  }
  size_ = 0;
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
template <typename... Args>
auto iumap<Key, Mapped, Size, Hash, KeyEqual>::try_emplace(Key const &key, Args &&...args)
    -> std::pair<iterator, bool> {
  auto *const slot = iumap::find_insert_slot(*this, key);
  if (slot == nullptr) {
    // The map is full and the key was not found. Insertion failed.
    return std::make_pair(this->end(), false);
  }
  auto const it = iterator{slot, &v_};
  auto const do_insert = slot->state == state::unused || slot->state == state::tombstone;
  if (do_insert) {
    // Not found. Add a new key/value pair.
    new (slot->kvp) value_type(key, std::forward<Args>(args)...);
    ++size_;
    slot->state = state::occupied;
  }
  return std::make_pair(it, do_insert);
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
auto iumap<Key, Mapped, Size, Hash, KeyEqual>::insert(value_type const &value) -> std::pair<iterator, bool> {
  return try_emplace(value.first, value.second);
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
template <typename M>
auto iumap<Key, Mapped, Size, Hash, KeyEqual>::insert_or_assign(Key const &key, M &&value)
    -> std::pair<iterator, bool> {
  auto *const slot = iumap::find_insert_slot(*this, key);
  if (slot == nullptr) {
    // The map is full and the key was not found. Insertion failed.
    return std::make_pair(this->end(), false);
  }
  if (slot->state == state::unused || slot->state == state::tombstone) {
    // Not found. Add a new key/value pair.
    new (slot->kvp) value_type(key, std::forward<M>(value));
    ++size_;
    slot->state = state::occupied;
    return std::make_pair(iterator{slot, &v_}, true);
  }
  auto *const kvp = slot->cast();
  kvp->second = value;  // Overwrite the existing value.
  return std::make_pair(iterator{slot, &v_}, false);
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
auto iumap<Key, Mapped, Size, Hash, KeyEqual>::find(Key const &k) const -> const_iterator {
  auto *const slot = iumap::lookup_slot(*this, k);
  if (slot == nullptr || slot->state != state::occupied) {
    return this->end();  // Not found
  }
  return {slot, &v_};  // Found
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
auto iumap<Key, Mapped, Size, Hash, KeyEqual>::find(Key const &k) -> iterator {
  auto *const slot = iumap::lookup_slot(*this, k);
  if (slot == nullptr || slot->state != state::occupied) {
    return this->end();  // Not found
  }
  return {slot, &v_};  // Found
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
template <typename Container>
auto *iumap<Key, Mapped, Size, Hash, KeyEqual>::lookup_slot(Container &container, Key const &key) {
  using slot_type = std::remove_pointer_t<decltype(container.v_.data())>;
  auto const size = container.v_.size();
  auto const equal = KeyEqual{};

  auto pos = Hash{}(key) % size;
  for (auto iteration = 1U; iteration <= size; ++iteration) {
    slot_type *const slot = &container.v_[pos];
    switch (slot->state) {
    case state::unused: return slot;
    case state::tombstone:
      // Keep searching.
      break;
    case state::occupied:
      if (equal(slot->cast()->first, key)) {
        return slot;
      }
      break;
    }
    pos = (pos + iteration) % size;
  }
  return (slot_type *){nullptr};  // No available slot.
}

template <typename Key, typename Mapped, std::size_t Size, typename Hash, typename KeyEqual>
template <typename Container>
auto *iumap<Key, Mapped, Size, Hash, KeyEqual>::find_insert_slot(Container &container, Key const &key) {
  using slot_type = std::remove_pointer_t<decltype(container.v_.data())>;
  auto const size = container.v_.size();
  auto const equal = KeyEqual{};

  auto pos = Hash{}(key) % size;
  slot_type *first_tombstone = nullptr;
  for (auto iteration = 1U; iteration <= size; ++iteration) {
    slot_type *slot = &container.v_[pos];
    switch (slot->state) {
    case state::tombstone:
      if (first_tombstone == nullptr) {
        first_tombstone = slot;
      }
      break;
    case state::occupied:
      if (equal(slot->cast()->first, key)) {
        return slot;
      }
      break;
    case state::unused: return first_tombstone != nullptr ? first_tombstone : slot;
    }
    pos = (pos + iteration) % size;
  }
  return first_tombstone != nullptr ? first_tombstone : (slot_type *){nullptr};
}

template <typename ValueType, std::size_t Size> struct lru_list {
  struct node {
    [[nodiscard]] constexpr ValueType *value() noexcept { return std::bit_cast<ValueType *>(&payload[0]); };
    [[nodiscard]] constexpr ValueType const *value() const noexcept {
      return std::bit_cast<ValueType const *>(&payload[0]);
    };

    alignas(ValueType) std::byte payload[sizeof(ValueType)]{};
    node *prev = nullptr;
    node *next = nullptr;
  };
  node *first_ = nullptr;
  node *last_ = nullptr;
  std::size_t size_ = 0;
  std::array<node, Size> v_;

  void clear();
  constexpr std::size_t size() const noexcept { return size_; }
  void touch(node *const n);

  static void null_evictor(ValueType *) {}

  template <typename Evictor = decltype(null_evictor)> node *add(ValueType &&payload, Evictor evictor = null_evictor);

  void check_invariants() const noexcept;
  void dump(std::ostream &os) const;
};

template <typename ValueType, std::size_t Size> void lru_list<ValueType, Size>::clear() {
  std::for_each(&v_[0], &v_[0] + size_, [](node *const n) {
    n->value()->~ValueType();  // evict the old value. Bye bye.
  });
  size_ = 0;
  first_ = nullptr;
  last_ = nullptr;
}

template <typename ValueType, std::size_t Size> void lru_list<ValueType, Size>::touch(node *const n) {
  assert(first_ != nullptr && last_ != nullptr);
  if (first_ == n) {
    return;
  }
  // Unhook 'n' from the list in its current position.
  if (last_ == n) {
    last_ = n->prev;
  }
  if (n->next != nullptr) {
    n->next->prev = n->prev;
  }
  if (n->prev != nullptr) {
    n->prev->next = n->next;
  }
  // Push on the front of the list.
  n->prev = nullptr;
  n->next = first_;
  first_->prev = n;
  first_ = n;
  this->check_invariants();
}

template <typename ValueType, std::size_t Size>
template <typename Evictor>
auto lru_list<ValueType, Size>::add(ValueType &&payload, Evictor evictor) -> node * {
  node *result = nullptr;
  if (size_ < v_.size()) {
    result = &v_[size_];
    new (result->value()) ValueType{std::forward<ValueType>(payload)};

    ++size_;
    if (last_ == nullptr) {
      last_ = result;
    }
  } else {
    assert(first_ != nullptr && last_ != nullptr);
    result = last_;
    evictor(result->value());
    // std::cout << "evict " << *(last_->value()) << '\n';
    *last_->value() = std::move(payload);

    last_ = result->prev;
    last_->next = nullptr;
  }

  result->prev = nullptr;
  result->next = first_;

  if (first_ != nullptr) {
    assert(first_->prev == nullptr);
    first_->prev = result;
  }
  first_ = result;
  this->check_invariants();
  return result;
}

template <typename ValueType, std::size_t Size> void lru_list<ValueType, Size>::check_invariants() const noexcept {
#ifndef NDEBUG
  assert((first_ == nullptr) == (size_ == 0) && "first_ must be null if and only if the container is empty");
  assert((first_ == last_) == (size_ < 2) && "with < 2 members, first_ and last_ must be equal");
  assert((first_ == nullptr || first_->prev == nullptr) && "prev of the first element must be null");
  assert((last_ == nullptr || last_->next == nullptr) && "next of the last element must be null");

  node const *prev = nullptr;
  for (auto const *n = first_; n != nullptr; n = n->next) {
    assert(n->prev == prev && "a node's next and prev pointers are inconsistent");
    prev = n;
  }
  assert(last_ == prev && "the last pointer is not correct");
#endif
}

template <typename ValueType, std::size_t Size> void lru_list<ValueType, Size>::dump(std::ostream &os) const {
  this->check_invariants();
  node const *prev = nullptr;
  char const *separator = "";
  for (node const *n = first_; n != nullptr; n = n->next) {
    os << separator << *(n->value());
    separator = " ";
    prev = n;
  }
  os << '\n';
}

#endif  // CACHE_HPP
