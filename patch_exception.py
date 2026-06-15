import sys

filename = "core/include/ikos/core/domain/exception/exception.hpp"
with open(filename, "r") as f:
    content = f.read()

taint_methods = """
  /// @}
  /// \\name Taint abstract domain methods
  /// @{

  void taint_assign_untainted(VariableRef x) override {
    if (this->is_bottom()) return;
    this->_normal.taint_assign_untainted(x);
  }

  void taint_assign_tainted(VariableRef x) override {
    if (this->is_bottom()) return;
    this->_normal.taint_assign_tainted(x);
  }

  void taint_assert_untainted(VariableRef x) override {
    if (this->is_bottom()) return;
    this->_normal.taint_assert_untainted(x);
  }

  void taint_assert_tainted(VariableRef x) override {
    if (this->is_bottom()) return;
    this->_normal.taint_assert_tainted(x);
  }

  bool taint_is_untainted(VariableRef x) const override {
    if (this->is_bottom()) return true;
    return this->_normal.taint_is_untainted(x);
  }

  bool taint_is_tainted(VariableRef x) const override {
    if (this->is_bottom()) return true;
    return this->_normal.taint_is_tainted(x);
  }

  void taint_set(VariableRef x, Taint value) override {
    if (this->is_bottom()) return;
    this->_normal.taint_set(x, value);
  }

  void taint_refine(VariableRef x, Taint value) override {
    if (this->is_bottom()) return;
    this->_normal.taint_refine(x, value);
  }

  Taint taint_to_taint(VariableRef x) const override {
    if (this->is_bottom()) return Taint::bottom();
    return this->_normal.taint_to_taint(x);
  }

"""
content = content.replace("  /// @}\n  /// \\name Pointer abstract domain methods", taint_methods + "  /// @}\n  /// \\name Pointer abstract domain methods")

with open(filename, "w") as f:
    f.write(content)

