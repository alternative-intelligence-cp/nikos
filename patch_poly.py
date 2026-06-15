import sys

filename = "core/include/ikos/core/domain/memory/polymorphic_domain.hpp"
with open(filename, "r") as f:
    content = f.read()

base_methods = """
  /// @}
  /// \\name Taint abstract domain methods
  /// @{

  virtual void taint_assign_untainted(VariableRef x) = 0;
  virtual void taint_assign_tainted(VariableRef x) = 0;
  virtual void taint_assert_untainted(VariableRef x) = 0;
  virtual void taint_assert_tainted(VariableRef x) = 0;
  virtual bool taint_is_untainted(VariableRef x) const = 0;
  virtual bool taint_is_tainted(VariableRef x) const = 0;
  virtual void taint_set(VariableRef x, Taint value) = 0;
  virtual void taint_refine(VariableRef x, Taint value) = 0;
  virtual Taint taint_to_taint(VariableRef x) const = 0;

"""

derived_methods = """
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

domain_methods = """
  /// @}
  /// \\name Taint abstract domain methods
  /// @{

  void taint_assign_untainted(VariableRef x) override {
    this->_ptr->taint_assign_untainted(x);
  }

  void taint_assign_tainted(VariableRef x) override {
    this->_ptr->taint_assign_tainted(x);
  }

  void taint_assert_untainted(VariableRef x) override {
    this->_ptr->taint_assert_untainted(x);
  }

  void taint_assert_tainted(VariableRef x) override {
    this->_ptr->taint_assert_tainted(x);
  }

  bool taint_is_untainted(VariableRef x) const override {
    return this->_ptr->taint_is_untainted(x);
  }

  bool taint_is_tainted(VariableRef x) const override {
    return this->_ptr->taint_is_tainted(x);
  }

  void taint_set(VariableRef x, Taint value) override {
    this->_ptr->taint_set(x, value);
  }

  void taint_refine(VariableRef x, Taint value) override {
    this->_ptr->taint_refine(x, value);
  }

  Taint taint_to_taint(VariableRef x) const override {
    return this->_ptr->taint_to_taint(x);
  }

"""

parts = content.split("  /// @}\n  /// \\name Pointer abstract domain methods")
if len(parts) == 4:
    new_content = parts[0] + base_methods + "  /// @}\n  /// \\name Pointer abstract domain methods" + parts[1] + derived_methods + "  /// @}\n  /// \\name Pointer abstract domain methods" + parts[2] + domain_methods + "  /// @}\n  /// \\name Pointer abstract domain methods" + parts[3]
    with open(filename, "w") as f:
        f.write(new_content)
else:
    print(f"Error parsing polymorphic_domain.hpp, got {len(parts)} parts")
    sys.exit(1)

