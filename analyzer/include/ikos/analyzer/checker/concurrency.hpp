#pragma once

#include <ikos/analyzer/checker/checker.hpp>
#include <ikos/core/domain/lockset.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <ikos/analyzer/analysis/memory_location.hpp>

namespace ikos {
namespace analyzer {

/// \brief Concurrency checker
///
/// This checker detects data races and deadlocks.
class ConcurrencyChecker final : public Checker {
private:
  using Lockset = core::LocksetDomain< MemoryLocation* >;

  /// \brief Map from statements to the lockset IN state
  boost::container::flat_map< ar::Statement*, Lockset > _stmt_locksets;

public:
  /// \brief Constructor
  explicit ConcurrencyChecker(Context& ctx);

  /// \brief Get the checker name
  CheckerName name() const override;

  /// \brief Get the checker description
  const char* description() const override;

  /// \brief Check a statement
  void check(ar::Statement* stmt,
             const value::AbstractDomain& inv,
             CallContext* call_context) override;

private:
  /// \brief Analyze a function to build _stmt_locksets
  void analyze_function(ar::Function* fun);

  /// \brief Check a memory access
  void check_memory_access(ar::Statement* stmt,
                           ar::Value* pointer,
                           const value::AbstractDomain& inv,
                           CallContext* call_context,
                           bool is_write);
};

} // end namespace analyzer
} // end namespace ikos
