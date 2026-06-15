#pragma once

#include <set>
#include <string>
#include <sstream>

#include <ikos/core/domain/abstract_domain.hpp>
#include <ikos/core/support/assert.hpp>

namespace ikos {
namespace core {

/// \brief Taint abstract value with optional provenance labels
///
/// Lattice: Bottom < Untainted < Tainted({labels}) < Top
///
/// A Tainted value carries a set of source labels (function names) from
/// which the taint originated. The label set is unioned on join operations,
/// enabling multi-source provenance tracking.
///
/// Backward compatibility: Taint::tainted() (no args) creates a tainted value
/// with an empty label set; Taint::tainted(label) creates labeled taint.
class Taint final : public core::AbstractDomain< Taint > {
private:
  enum Kind : unsigned {
    BottomKind    = 0,
    UntaintedKind = 1,
    TaintedKind   = 2,
    TopKind       = 3
  };

private:
  Kind _kind = TopKind;

  /// \brief Provenance labels (meaningful only when _kind == TaintedKind)
  std::set< std::string > _labels;

private:
  explicit Taint(Kind kind) : _kind(kind) {}

  Taint(Kind kind, std::set< std::string > labels)
      : _kind(kind), _labels(std::move(labels)) {}

public:
  static Taint top()       { return Taint(TopKind); }
  static Taint bottom()    { return Taint(BottomKind); }
  static Taint untainted() { return Taint(UntaintedKind); }

  /// \brief Return a tainted value without a source label (backward compat)
  static Taint tainted() { return Taint(TaintedKind); }

  /// \brief Return a tainted value with a specific source label
  static Taint tainted(std::string label) {
    std::set< std::string > s;
    s.insert(std::move(label));
    return Taint(TaintedKind, std::move(s));
  }

  /// \brief Return a tainted value with a set of source labels
  static Taint tainted(std::set< std::string > labels) {
    return Taint(TaintedKind, std::move(labels));
  }

  Taint(const Taint&)     = default;
  Taint(Taint&&) noexcept = default;
  Taint& operator=(const Taint&)     = default;
  Taint& operator=(Taint&&) noexcept = default;
  ~Taint() override = default;

  void normalize() override {}

  bool is_bottom()    const override { return this->_kind == BottomKind; }
  bool is_top()       const override { return this->_kind == TopKind; }
  bool is_untainted() const          { return this->_kind == UntaintedKind; }
  bool is_tainted()   const          { return this->_kind == TaintedKind; }

  void set_to_bottom() override { this->_kind = BottomKind; this->_labels.clear(); }
  void set_to_top()    override { this->_kind = TopKind;    this->_labels.clear(); }

  void set_to_untainted() { this->_kind = UntaintedKind; this->_labels.clear(); }
  void set_to_tainted()   { this->_kind = TaintedKind; }

  /// \brief Return the provenance labels (non-empty only when is_tainted())
  const std::set< std::string >& labels() const { return this->_labels; }

  /// \brief Add a provenance label (no-op unless is_tainted())
  void add_label(std::string label) {
    if (this->_kind == TaintedKind) {
      this->_labels.insert(std::move(label));
    }
  }

  bool leq(const Taint& other) const override {
    switch (this->_kind) {
      case BottomKind:    return true;
      case TopKind:       return other._kind == TopKind;
      case UntaintedKind: return other._kind == UntaintedKind || other._kind == TopKind;
      case TaintedKind:   return other._kind == TaintedKind   || other._kind == TopKind;
      default:            ikos_unreachable("unreachable");
    }
  }

  bool equals(const Taint& other) const override {
    return this->_kind == other._kind && this->_labels == other._labels;
  }

  void join_with(const Taint& other) override {
    this->_kind = static_cast< Kind >(
        static_cast< unsigned >(this->_kind) |
        static_cast< unsigned >(other._kind));
    if (this->_kind == TaintedKind) {
      // Union label sets
      for (const auto& lbl : other._labels) {
        this->_labels.insert(lbl);
      }
    } else {
      this->_labels.clear();
    }
  }

  void widen_with(const Taint& other) override { this->join_with(other); }

  void meet_with(const Taint& other) override {
    this->_kind = static_cast< Kind >(
        static_cast< unsigned >(this->_kind) &
        static_cast< unsigned >(other._kind));
    if (this->_kind == TaintedKind) {
      // Intersect label sets
      std::set< std::string > intersected;
      for (const auto& lbl : this->_labels) {
        if (other._labels.count(lbl)) {
          intersected.insert(lbl);
        }
      }
      this->_labels = std::move(intersected);
    } else {
      this->_labels.clear();
    }
  }

  void narrow_with(const Taint& other) override { this->meet_with(other); }

  void dump(std::ostream& o) const override {
    switch (this->_kind) {
      case BottomKind:    o << "bottom"; break;
      case UntaintedKind: o << "U"; break;
      case TaintedKind: {
        if (this->_labels.empty()) {
          o << "T";
        } else {
          o << "T{";
          bool first = true;
          for (const auto& lbl : this->_labels) {
            if (!first) o << ",";
            o << lbl;
            first = false;
          }
          o << "}";
        }
        break;
      }
      case TopKind: o << "Top"; break;
      default: ikos_unreachable("unreachable");
    }
  }

  static std::string name() { return "taint"; }

}; // end class Taint

} // end namespace core
} // end namespace ikos
