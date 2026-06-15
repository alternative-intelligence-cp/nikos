#pragma once

#include <ikos/analyzer/checker/checker.hpp>
#include <ikos/analyzer/checker/taint_config.hpp>

namespace ikos {
namespace analyzer {

/// \brief Taint checker
class TaintChecker final : public Checker {
public:
  /// \brief Constructor
  explicit TaintChecker(Context& ctx);

  /// \brief Get the checker name
  CheckerName name() const override;

  /// \brief Get the checker description
  const char* description() const override;

  /// \brief Check a statement
  void check(ar::Statement* stmt,
             const value::AbstractDomain& inv,
             CallContext* call_context) override;

private:
  /// \brief Check result
  struct CheckResult {
    CheckKind kind;
    Result result;
    JsonDict info;
  };

  /// \brief Check a function call against taint rules
  CheckResult check_call(ar::CallBase* call,
                         const value::AbstractDomain& inv,
                         const TaintConfig::Sink* sink);

  /// \brief Display the check for the given call, if requested
  std::optional< LogMessage > display_taint_check(
      Result result, ar::CallBase* stmt) const;

}; // end class TaintChecker

} // end namespace analyzer
} // end namespace ikos
