//
//  lru_list.hpp
//  hasht
//
//  Created by Paul Bowen-Huggett on 24/10/2024.
//

#ifndef LRU_LIST_HPP
#define LRU_LIST_HPP

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <new>
#include <utility>

#ifndef NDEBUG
#include <ostream>
#endif

template <typename ValueType, std::size_t Size> class lru_list {
public:
  class node {
    friend class lru_list;

  public:
    [[nodiscard]] constexpr ValueType *value() noexcept { return std::bit_cast<ValueType *>(&payload[0]); }
    [[nodiscard]] constexpr ValueType const *value() const noexcept {
      return std::bit_cast<ValueType const *>(&payload[0]);
    }

  private:
    alignas(ValueType) std::byte payload[sizeof(ValueType)]{};
    node *prev = nullptr;
    node *next = nullptr;
  };

  void clear();
  [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
  void touch(node *const n);

  template <typename Evictor = void(ValueType *)> node *add(ValueType &&payload, Evictor evictor);

#ifndef NDEBUG
  void dump(std::ostream &os) const;
#endif

private:
  node *first_ = nullptr;
  node *last_ = nullptr;
  std::size_t size_ = 0;
  std::array<node, Size> v_;

  void check_invariants() const noexcept;
};

// clear
// ~~~~~
template <typename ValueType, std::size_t Size> void lru_list<ValueType, Size>::clear() {
  std::for_each(&v_[0], &v_[0] + size_, [](node *const n) {
    n->value()->~ValueType();  // Evict the old value. Bye bye.
  });
  size_ = 0;
  first_ = nullptr;
  last_ = nullptr;
}

// touch
// ~~~~~
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

// add
// ~~~
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

// check invariants
// ~~~~~~~~~~~~~~~~
template <typename ValueType, std::size_t Size> void lru_list<ValueType, Size>::check_invariants() const noexcept {
#ifndef NDEBUG
  assert((first_ == nullptr) == (size_ == 0) && "first_ must be null if and only if the container is empty");
  assert((first_ == last_) == (size_ < 2) && "with < 2 members, first_ and last_ must be equal");
  assert((first_ == nullptr || first_->prev == nullptr) && "prev of the first element must be null");
  assert((last_ == nullptr || last_->next == nullptr) && "next of the last element must be null");

  node const *prev = nullptr;
  for (auto const *n = first_; n != nullptr; n = n->next) {
    assert(n->prev == prev && "next and prev pointers are inconsistent between two nodes");
    prev = n;
  }
  assert(last_ == prev && "the last pointer is not correct");
#endif
}

// dump
// ~~~~
#ifndef NDEBUG
template <typename ValueType, std::size_t Size> void lru_list<ValueType, Size>::dump(std::ostream &os) const {
  this->check_invariants();
  char const *separator = "";
  for (node const *n = first_; n != nullptr; n = n->next) {
    os << separator << *(n->value());
    separator = " ";
  }
  os << '\n';
}
#endif

#endif  // LRU_LIST_HPP