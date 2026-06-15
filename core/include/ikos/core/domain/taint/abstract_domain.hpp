#pragma once

#include <set>
#include <string>

#include <ikos/core/domain/abstract_domain.hpp>
#include <ikos/core/semantic/variable.hpp>
#include <ikos/core/value/taint.hpp>

namespace ikos {
namespace core {
namespace taint {

/// \brief Base class for abstract domains keeping track of taint
template < typename VariableRef, typename Derived >
class AbstractDomain : public core::AbstractDomain< Derived > {
public:
  static_assert(
      core::IsVariable< VariableRef >::value,
      "VariableRef does not meet the requirements for variable types");

public:
  /// \brief Assign `x = untainted`
  virtual void assign_untainted(VariableRef x) = 0;

  /// \brief Assign `x = tainted` (no provenance label)
  virtual void assign_tainted(VariableRef x) = 0;

  /// \brief Assign `x = tainted` with a specific source label for provenance
  virtual void assign_labeled(VariableRef x, std::string label) {
    this->assign_tainted(x);
    Taint t = this->get(x);
    t.add_label(std::move(label));
    this->set(x, t);
  }

  /// \brief Assign `x = y`
  virtual void assign(VariableRef x, VariableRef y) = 0;

  /// \brief Assign `x = op(y, z)`
  virtual void apply(VariableRef x, VariableRef y, VariableRef z) = 0;

  /// \brief Assign `x = op(y)`
  virtual void apply(VariableRef x, VariableRef y) = 0;

  /// \brief Add the constraint `x == untainted`
  virtual void assert_untainted(VariableRef x) = 0;

  /// \brief Add the constraint `x == tainted`
  virtual void assert_tainted(VariableRef x) = 0;

  /// \brief Return true if `x` is untainted, otherwise false
  virtual bool is_untainted(VariableRef x) const = 0;

  /// \brief Return true if `x` is tainted, otherwise false
  virtual bool is_tainted(VariableRef x) const = 0;

  /// \brief Set the taint value of a variable
  virtual void set(VariableRef x, const Taint& value) = 0;

  /// \brief Refine the taint value of a variable
  virtual void refine(VariableRef x, const Taint& value) = 0;

  /// \brief Forget the taint of a variable
  virtual void forget(VariableRef x) = 0;

  /// \brief Get the taint value for the given variable
  virtual Taint get(VariableRef x) const = 0;

}; // end class AbstractDomain

/// \brief Check if a type is a taint abstract domain
template < typename T, typename VariableRef >
struct IsAbstractDomain
    : std::is_base_of< taint::AbstractDomain< VariableRef, T >, T > {};

} // end namespace taint
} // end namespace core
} // end namespace ikos
