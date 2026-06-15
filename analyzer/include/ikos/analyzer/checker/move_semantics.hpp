/*******************************************************************************
 *
 * \file
 * \brief Use-after-move checker
 *
 * Author: NIKOS Development Team
 *
 * Contact: ikos@lists.nasa.gov
 *
 * Notices:
 *
 * Copyright (c) 2024 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Disclaimers:
 *
 * No Warranty: THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF
 * ANY KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO SPECIFICATIONS,
 * ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL BE
 * ERROR FREE, OR ANY WARRANTY THAT DOCUMENTATION, IF PROVIDED, WILL CONFORM TO
 * THE SUBJECT SOFTWARE. THIS AGREEMENT DOES NOT, IN ANY MANNER, CONSTITUTE AN
 * ENDORSEMENT BY GOVERNMENT AGENCY OR ANY PRIOR RECIPIENT OF ANY RESULTS,
 * RESULTING DESIGNS, HARDWARE, SOFTWARE PRODUCTS OR ANY OTHER APPLICATIONS
 * RESULTING FROM USE OF THE SUBJECT SOFTWARE.  FURTHER, GOVERNMENT AGENCY
 * DISCLAIMS ALL WARRANTIES AND LIABILITIES REGARDING THIRD-PARTY SOFTWARE,
 * IF PRESENT IN THE ORIGINAL SOFTWARE, AND DISTRIBUTES IT "AS IS."
 *
 * Waiver and Indemnity:  RECIPIENT AGREES TO WAIVE ANY AND ALL CLAIMS AGAINST
 * THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL
 * AS ANY PRIOR RECIPIENT.  IF RECIPIENT'S USE OF THE SUBJECT SOFTWARE RESULTS
 * IN ANY LIABILITIES, DEMANDS, DAMAGES, EXPENSES OR LOSSES ARISING FROM SUCH
 * USE, INCLUDING ANY DAMAGES FROM PRODUCTS BASED ON, OR RESULTING FROM,
 * RECIPIENT'S USE OF THE SUBJECT SOFTWARE, RECIPIENT SHALL INDEMNIFY AND HOLD
 * HARMLESS THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS,
 * AS WELL AS ANY PRIOR RECIPIENT, TO THE EXTENT PERMITTED BY LAW.
 * RECIPIENT'S SOLE REMEDY FOR ANY SUCH MATTER SHALL BE THE IMMEDIATE,
 * UNILATERAL TERMINATION OF THIS AGREEMENT.
 *
 ******************************************************************************/

#pragma once

#include <unordered_set>

#include <ikos/analyzer/checker/checker.hpp>

namespace ikos {
namespace analyzer {

/// \brief Use-after-move checker
///
/// Detects use of moved-from `std::unique_ptr` (and similar move-only types)
/// after the object has been moved from. After `std::move(p)`, the internal
/// pointer of `p` is set to null by the move constructor; any subsequent
/// dereference of `p` constitutes undefined behaviour.
///
/// Strategy:
/// - Intercept calls to `LibcppUniquePtrMove` intrinsic (mapped from
///   `_ZNSt10unique_ptr...C2EOS...` / `...aSEOS...` mangled names).
/// - The *source* argument (arg 1) is the unique_ptr being moved from.
///   Record every memory location in its points-to set in `_moved_from`.
/// - On every `Load` / `Store`, if the operand pointer's points-to set
///   intersects `_moved_from`, emit a `UseAfterMove` warning.
///
/// \note The execution engine already sets the moved-from unique_ptr's
/// internal field to null via the generic `exec_unknown_call` path for
/// `LibcppUniquePtrMove`. The nullity checker will catch the resulting null
/// dereference if there is a direct `*p` after `std::move(p)`. This checker
/// provides an *earlier*, more descriptive diagnostic that names the operation
/// as "use after move" rather than "null pointer dereference".
class MoveChecker final : public Checker {
private:
  using PointsToSet = core::PointsToSet< MemoryLocation* >;

  /// Set of memory locations that have been moved from
  std::unordered_set< MemoryLocation* > _moved_from;

public:
  /// \brief Constructor
  explicit MoveChecker(Context& ctx);

  /// \brief Get the checker name
  CheckerName name() const override;

  /// \brief Get the checker description
  const char* description() const override;

  /// \brief Check a statement
  void check(ar::Statement* stmt,
             const value::AbstractDomain& inv,
             CallContext* call_context) override;

private:
  struct CheckResult {
    CheckKind kind;
    Result result;
    ar::Value* operand;
    JsonDict info;
  };

  /// \brief Record the source argument of a move call as moved-from
  void record_move(ar::CallBase* call, const value::AbstractDomain& inv);

  /// \brief Check a memory access through the given pointer for use-after-move
  CheckResult check_mem_access(ar::Statement* stmt,
                               ar::Value* pointer,
                               bool is_write,
                               const value::AbstractDomain& inv);

  /// \brief Display a use-after-move diagnostic
  std::optional< LogMessage > display_uam_check(Result result,
                                                ar::Statement* stmt,
                                                ar::Value* pointer) const;
};

} // end namespace analyzer
} // end namespace ikos
