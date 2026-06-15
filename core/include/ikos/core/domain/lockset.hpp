#pragma once

#include <boost/optional.hpp>

#include <ikos/core/adt/patricia_tree/set.hpp>
#include <ikos/core/domain/abstract_domain.hpp>

namespace ikos {
namespace core {

/// \brief Lockset abstract domain
///
/// Tracks the set of held locks.
/// join = intersection (if a lock is held on path A but not path B, it's not held after the merge)
/// meet = union
template < typename MemoryLocationRef >
class LocksetDomain final : public AbstractDomain< LocksetDomain< MemoryLocationRef > > {
private:
  using PatriciaTreeSetT = PatriciaTreeSet< MemoryLocationRef >;

public:
  using Iterator = typename PatriciaTreeSetT::Iterator;

private:
  PatriciaTreeSetT _set;
  bool _is_bottom;

  struct BottomTag {};
  explicit LocksetDomain(BottomTag) : _is_bottom(true) {}

  struct TopTag {};
  explicit LocksetDomain(TopTag) : _is_bottom(false) {}

public:
  static LocksetDomain top() { return LocksetDomain(TopTag{}); }
  static LocksetDomain bottom() { return LocksetDomain(BottomTag{}); }

  LocksetDomain() = delete;
  LocksetDomain(const LocksetDomain&) noexcept = default;
  LocksetDomain(LocksetDomain&&) noexcept = default;
  LocksetDomain& operator=(const LocksetDomain&) noexcept = default;
  LocksetDomain& operator=(LocksetDomain&&) noexcept = default;
  ~LocksetDomain() override = default;

  void normalize() override {}

  bool is_bottom() const override { return this->_is_bottom; }
  bool is_top() const override { return !this->_is_bottom && this->_set.empty(); }

  void set_to_bottom() override {
    this->_is_bottom = true;
    this->_set.clear();
  }

  void set_to_top() override {
    this->_is_bottom = false;
    this->_set.clear();
  }

  bool leq(const LocksetDomain& other) const override {
    if (this->is_bottom()) return true;
    if (other.is_bottom()) return false;
    // Lockset L1 <= L2 means L1 has MORE or EQUAL locks than L2
    return other._set.is_subset_of(this->_set);
  }

  bool equals(const LocksetDomain& other) const override {
    if (this->is_bottom()) return other.is_bottom();
    if (other.is_bottom()) return false;
    return this->_set.equals(other._set);
  }

  void join_with(const LocksetDomain& other) override {
    if (this->is_bottom()) {
      this->operator=(other);
    } else if (other.is_bottom()) {
      return;
    } else {
      // join is intersection for locksets
      this->_set.intersect_with(other._set);
    }
  }

  void widen_with(const LocksetDomain& other) override {
    this->join_with(other);
  }

  void meet_with(const LocksetDomain& other) override {
    if (this->is_bottom() || other.is_bottom()) {
      this->set_to_bottom();
    } else {
      // meet is union
      this->_set.join_with(other._set);
    }
  }

  void narrow_with(const LocksetDomain& other) override {
    this->meet_with(other);
  }

  void add(MemoryLocationRef lock) {
    if (this->is_bottom()) return;
    this->_set.insert(lock);
  }

  void remove(MemoryLocationRef lock) {
    if (this->is_bottom()) return;
    this->_set.erase(lock);
  }

  bool contains(MemoryLocationRef lock) const {
    if (this->is_bottom()) return true;
    return this->_set.contains(lock);
  }

  bool empty() const {
    if (this->is_bottom()) return false;
    return this->_set.empty();
  }

  Iterator begin() const {
    ikos_assert(!this->is_bottom());
    return this->_set.begin();
  }

  Iterator end() const {
    ikos_assert(!this->is_bottom());
    return this->_set.end();
  }

  void dump(std::ostream& o) const override {
    if (this->is_bottom()) {
      o << "⊥";
    } else if (this->is_top()) {
      o << "{} (top)";
    } else {
      this->_set.dump(o);
    }
  }

  static std::string name() { return "lockset domain"; }
};

} // end namespace core
} // end namespace ikos
