import sys

def patch_file(filename, is_polymorphic=False):
    with open(filename, "r") as f:
        content = f.read()

    taint_methods = """
  /// @}
  /// \\name Taint abstract domain methods
  /// @{

  void taint_assign_untainted(VariableRef x) override {
    this->_inv.taint_assign_untainted(x);
  }

  void taint_assign_tainted(VariableRef x) override {
    this->_inv.taint_assign_tainted(x);
  }

  void taint_assert_untainted(VariableRef x) override {
    this->_inv.taint_assert_untainted(x);
  }

  void taint_assert_tainted(VariableRef x) override {
    this->_inv.taint_assert_tainted(x);
  }

  bool taint_is_untainted(VariableRef x) const override {
    return this->_inv.taint_is_untainted(x);
  }

  bool taint_is_tainted(VariableRef x) const override {
    return this->_inv.taint_is_tainted(x);
  }

  void taint_set(VariableRef x, Taint value) override {
    this->_inv.taint_set(x, value);
  }

  void taint_refine(VariableRef x, Taint value) override {
    this->_inv.taint_refine(x, value);
  }

  Taint taint_to_taint(VariableRef x) const override {
    return this->_inv.taint_to_taint(x);
  }

"""
    if is_polymorphic:
        taint_methods = taint_methods.replace("this->_inv.", "this->_impl->")
    
    content = content.replace("  /// @}\n  /// \\name Pointer abstract domain methods", taint_methods + "  /// @}\n  /// \\name Pointer abstract domain methods")

    with open(filename, "w") as f:
        f.write(content)

patch_file("core/include/ikos/core/domain/memory/value.hpp")
patch_file("core/include/ikos/core/domain/memory/partitioning.hpp")
patch_file("core/include/ikos/core/domain/memory/polymorphic_domain.hpp", True)

