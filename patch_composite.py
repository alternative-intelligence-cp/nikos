import sys

with open("core/include/ikos/core/domain/scalar/composite.hpp", "r") as f:
    content = f.read()

# 1. Add TaintDomain to template parameters
content = content.replace("typename MachineIntDomain,\n           typename NullityDomain >",
                          "typename MachineIntDomain,\n           typename NullityDomain,\n           typename TaintDomain >")

content = content.replace("MachineIntDomain,\n                                                      NullityDomain > >",
                          "MachineIntDomain,\n                                                      NullityDomain,\n                                                      TaintDomain > >")

content = content.replace("NullityDomain >::value", "NullityDomain >::value") # Leave this, but we should add static_assert for TaintDomain

# Add static_assert for TaintDomain
assert_taint = """  static_assert(taint::IsAbstractDomain< TaintDomain, VariableRef >::value,
                "TaintDomain must implement taint::AbstractDomain");
"""
content = content.replace("public:\n  using IntUnaryOperator", assert_taint + "\npublic:\n  using IntUnaryOperator")

# 2. Add member variable _taint
content = content.replace("  /// \\brief Underlying nullity abstract domains\n  NullityDomain _nullity;",
                          "  /// \\brief Underlying nullity abstract domains\n  NullityDomain _nullity;\n\n  /// \\brief Underlying taint abstract domains\n  TaintDomain _taint;")

# 3. Update constructors
content = content.replace("MachineIntDomain integer,\n                  NullityDomain nullity,\n                  PointsToMap points_to_map)",
                          "MachineIntDomain integer,\n                  NullityDomain nullity,\n                  TaintDomain taint,\n                  PointsToMap points_to_map)")

content = content.replace("_nullity(std::move(nullity)),\n        _points_to_map(std::move(points_to_map))",
                          "_nullity(std::move(nullity)),\n        _taint(std::move(taint)),\n        _points_to_map(std::move(points_to_map))")

content = content.replace("MachineIntDomain integer,\n                  NullityDomain nullity)",
                          "MachineIntDomain integer,\n                  NullityDomain nullity,\n                  TaintDomain taint)")

content = content.replace("_nullity(std::move(nullity)),\n        _points_to_map(PointsToMap::top())",
                          "_nullity(std::move(nullity)),\n        _taint(std::move(taint)),\n        _points_to_map(PointsToMap::top())")

# Update noexcept clauses
content = content.replace("&& (std::is_nothrow_copy_constructible< NullityDomain >::value))",
                          "&& (std::is_nothrow_copy_constructible< NullityDomain >::value) && (std::is_nothrow_copy_constructible< TaintDomain >::value))")
content = content.replace("&& (std::is_nothrow_move_constructible< NullityDomain >::value))",
                          "&& (std::is_nothrow_move_constructible< NullityDomain >::value) && (std::is_nothrow_move_constructible< TaintDomain >::value))")
content = content.replace("&& (std::is_nothrow_copy_assignable< NullityDomain >::value))",
                          "&& (std::is_nothrow_copy_assignable< NullityDomain >::value) && (std::is_nothrow_copy_assignable< TaintDomain >::value))")
content = content.replace("&& (std::is_nothrow_move_assignable< NullityDomain >::value))",
                          "&& (std::is_nothrow_move_assignable< NullityDomain >::value) && (std::is_nothrow_move_assignable< TaintDomain >::value))")

# 4. Update core methods (normalize, is_bottom, is_top, set_to_bottom, set_to_top, leq, equals, joins, meets, etc.)
content = content.replace("this->_nullity.normalize();", "this->_nullity.normalize();\n    if (this->_nullity.is_bottom()) {\n      this->set_to_bottom();\n      return;\n    }\n\n    this->_taint.normalize();")
content = content.replace("this->_nullity.is_bottom() ||", "this->_nullity.is_bottom() || this->_taint.is_bottom() ||")
content = content.replace("this->_nullity.is_top() &&", "this->_nullity.is_top() && this->_taint.is_top() &&")
content = content.replace("this->_nullity.set_to_bottom();", "this->_nullity.set_to_bottom();\n    this->_taint.set_to_bottom();")
content = content.replace("this->_nullity.set_to_top();", "this->_nullity.set_to_top();\n    this->_taint.set_to_top();")
content = content.replace("this->_nullity.leq(other._nullity) &&", "this->_nullity.leq(other._nullity) &&\n             this->_taint.leq(other._taint) &&")
content = content.replace("this->_nullity.equals(other._nullity) &&", "this->_nullity.equals(other._nullity) &&\n             this->_taint.equals(other._taint) &&")

# replace _nullity with _taint in joins/meets
methods = ['join_with', 'join_loop_with', 'join_iter_with', 'widen_with', 'meet_with', 'narrow_with']
for m in methods:
    content = content.replace(f"this->_nullity.{m}(std::move(other._nullity));", f"this->_nullity.{m}(std::move(other._nullity));\n      this->_taint.{m}(std::move(other._taint));")
    content = content.replace(f"this->_nullity.{m}(other._nullity);", f"this->_nullity.{m}(other._nullity);\n      this->_taint.{m}(other._taint);")

# widen_threshold_with, narrow_threshold_with
content = content.replace("this->_nullity.widen_with(other._nullity);", "this->_nullity.widen_with(other._nullity);\n      this->_taint.widen_with(other._taint);")
content = content.replace("this->_nullity.narrow_with(other._nullity);", "this->_nullity.narrow_with(other._nullity);\n      this->_taint.narrow_with(other._taint);")

# return CompositeDomain(...)
content = content.replace("this->_nullity.join(other._nullity),", "this->_nullity.join(other._nullity),\n                             this->_taint.join(other._taint),")
content = content.replace("this->_nullity.join_loop(other._nullity),", "this->_nullity.join_loop(other._nullity),\n                             this->_taint.join_loop(other._taint),")
content = content.replace("this->_nullity.join_iter(other._nullity),", "this->_nullity.join_iter(other._nullity),\n                             this->_taint.join_iter(other._taint),")
content = content.replace("this->_nullity.widening(other._nullity),", "this->_nullity.widening(other._nullity),\n                             this->_taint.widening(other._taint),")
content = content.replace("this->_nullity.meet(other._nullity),", "this->_nullity.meet(other._nullity),\n                             this->_taint.meet(other._taint),")
content = content.replace("this->_nullity.narrowing(other._nullity),", "this->_nullity.narrowing(other._nullity),\n                             this->_taint.narrowing(other._taint),")

# 5. Add taint_* methods
taint_methods = """
  /// @}
  /// \\name Taint abstract domain methods
  /// @{

  void taint_assign_untainted(VariableRef x) override {
    this->_taint.assign_untainted(x);
  }

  void taint_assign_tainted(VariableRef x) override {
    this->_taint.assign_tainted(x);
  }

  void taint_assert_untainted(VariableRef x) override {
    this->_taint.assert_untainted(x);
  }

  void taint_assert_tainted(VariableRef x) override {
    this->_taint.assert_tainted(x);
  }

  bool taint_is_untainted(VariableRef x) const override {
    return this->_taint.is_untainted(x);
  }

  bool taint_is_tainted(VariableRef x) const override {
    return this->_taint.is_tainted(x);
  }

  void taint_set(VariableRef x, Taint value) override {
    this->_taint.set(x, value);
  }

  void taint_refine(VariableRef x, Taint value) override {
    this->_taint.refine(x, value);
  }

  Taint taint_to_taint(VariableRef x) const override {
    return this->_taint.get(x);
  }

"""
content = content.replace("  /// @}\n  /// \\name Pointer abstract domain methods", taint_methods + "  /// @}\n  /// \\name Pointer abstract domain methods")

# 6. Update dynamic_assign, dynamic_forget, scalar_forget, dynamic_write_*, dynamic_read_*
content = content.replace("this->_nullity.forget(x);", "this->_nullity.forget(x);\n    this->_taint.forget(x);")
content = content.replace("this->_nullity.assign(x, y);", "this->_nullity.assign(x, y);\n    this->_taint.assign(x, y);")
content = content.replace("this->_nullity.set(x, Nullity::top());", "this->_nullity.set(x, Nullity::top());\n    this->_taint.set(x, Taint::top());")
content = content.replace("this->_nullity.set(x, Nullity::bottom());", "this->_nullity.set(x, Nullity::bottom());\n    this->_taint.set(x, Taint::bottom());")
content = content.replace("this->_nullity.assign_null(x);", "this->_nullity.assign_null(x);\n    this->_taint.assign_untainted(x);")

# For integer, floating point assignments
content = content.replace("this->_integer.assign(x, n);", "this->_integer.assign(x, n);\n    this->_taint.assign_untainted(x);")
content = content.replace("this->_integer.forget(x);", "this->_integer.forget(x);\n    this->_taint.forget(x);")

# Also include <ikos/core/domain/taint/abstract_domain.hpp>
content = content.replace("#include <ikos/core/domain/uninitialized/abstract_domain.hpp>", "#include <ikos/core/domain/uninitialized/abstract_domain.hpp>\n#include <ikos/core/domain/taint/abstract_domain.hpp>")

with open("core/include/ikos/core/domain/scalar/composite.hpp", "w") as f:
    f.write(content)
