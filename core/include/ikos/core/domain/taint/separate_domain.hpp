#pragma once

#include <ikos/core/domain/taint/abstract_domain.hpp>
#include <ikos/core/domain/separate_domain.hpp>

namespace ikos {
namespace core {
namespace taint {

/// \brief Taint abstract domain
///
/// Implementation of the taint abstract domain interface using a separate
/// domain.
template < typename VariableRef >
class SeparateDomain final
    : public taint::AbstractDomain< VariableRef,
                                    SeparateDomain< VariableRef > > {
private:
  using SeparateDomainT = core::SeparateDomain< VariableRef, Taint >;

public:
  using Iterator = typename SeparateDomainT::Iterator;

private:
  SeparateDomainT _inv;

private:
  /// \brief Private constructor
  explicit SeparateDomain(SeparateDomainT inv) : _inv(std::move(inv)) {}

public:
  /// \brief Create the top abstract value
  static SeparateDomain top() { return SeparateDomain(SeparateDomainT::top()); }

  /// \brief Create the bottom abstract value
  static SeparateDomain bottom() {
    return SeparateDomain(SeparateDomainT::bottom());
  }

  /// \brief Copy constructor
  SeparateDomain(const SeparateDomain&) noexcept = default;

  /// \brief Move constructor
  SeparateDomain(SeparateDomain&&) noexcept = default;

  /// \brief Copy assignment operator
  SeparateDomain& operator=(const SeparateDomain&) noexcept = default;

  /// \brief Move assignment operator
  SeparateDomain& operator=(SeparateDomain&&) noexcept = default;

  /// \brief Destructor
  ~SeparateDomain() override = default;

  /// \brief Begin iterator over the pairs (variable, taint)
  Iterator begin() const { return this->_inv.begin(); }

  /// \brief End iterator over the pairs (variable, taint)
  Iterator end() const { return this->_inv.end(); }

  void normalize() override {}

  bool is_bottom() const override { return this->_inv.is_bottom(); }

  bool is_top() const override { return this->_inv.is_top(); }

  void set_to_bottom() override { this->_inv.set_to_bottom(); }

  void set_to_top() override { this->_inv.set_to_top(); }

  bool leq(const SeparateDomain& other) const override {
    return this->_inv.leq(other._inv);
  }

  bool equals(const SeparateDomain& other) const override {
    return this->_inv.equals(other._inv);
  }

  void join_with(const SeparateDomain& other) override {
    this->_inv.join_with(other._inv);
  }

  void widen_with(const SeparateDomain& other) override {
    this->_inv.widen_with(other._inv);
  }

  void meet_with(const SeparateDomain& other) override {
    this->_inv.meet_with(other._inv);
  }

  void narrow_with(const SeparateDomain& other) override {
    this->_inv.narrow_with(other._inv);
  }

  void assign_untainted(VariableRef x) override {
    this->_inv.set(x, Taint::untainted());
  }

  void assign_tainted(VariableRef x) override {
    this->_inv.set(x, Taint::tainted());
  }

  void assign(VariableRef x, VariableRef y) override {
    this->_inv.set(x, this->_inv.get(y));
  }

  void apply(VariableRef x, VariableRef y, VariableRef z) override {
    if (this->is_bottom()) return;
    Taint yt = this->_inv.get(y);
    Taint zt = this->_inv.get(z);
    
    // Taint propagates through operations
    Taint xt = Taint::bottom();
    xt.join_with(yt);
    xt.join_with(zt);
    this->_inv.set(x, xt);
  }

  void apply(VariableRef x, VariableRef y) override {
    if (this->is_bottom()) return;
    this->_inv.set(x, this->_inv.get(y));
  }

  void assert_untainted(VariableRef x) override {
    this->_inv.refine(x, Taint::untainted());
  }

  void assert_tainted(VariableRef x) override {
    this->_inv.refine(x, Taint::tainted());
  }

  bool is_untainted(VariableRef x) const override {
    Taint value = this->_inv.get(x);
    return value.is_bottom() || value.is_untainted();
  }

  bool is_tainted(VariableRef x) const override {
    Taint value = this->_inv.get(x);
    return value.is_bottom() || value.is_tainted();
  }

  void set(VariableRef x, const Taint& value) override {
    this->_inv.set(x, value);
  }

  void refine(VariableRef x, const Taint& value) override {
    this->_inv.refine(x, value);
  }

  void forget(VariableRef x) override { this->_inv.forget(x); }

  Taint get(VariableRef x) const override { return this->_inv.get(x); }

  void dump(std::ostream& o) const override { return this->_inv.dump(o); }

  static std::string name() { return "taint domain"; }

}; // end class SeparateDomain

} // end namespace taint
} // end namespace core
} // end namespace ikos
