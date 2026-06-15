/*******************************************************************************
 *
 * \file
 * \brief Use-after-free checker
 *
 * Author: NIKOS Development Team
 *
 * Contact: ikos@lists.nasa.gov
 *
 * Notices:
 *
 * Copyright (c) 2011-2024 United States Government as represented by the
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

#include <ikos/analyzer/checker/checker.hpp>

namespace ikos {
namespace analyzer {

/// \brief Use-after-free checker
///
/// This checker is a dedicated, focused analysis for use-after-free and
/// use-after-return bugs. Unlike the buffer overflow checker (boa), which
/// detects UAF as a side-effect of bounds checking, this checker is invoked
/// as `-a uaf` and only emits UAF/UAR findings.
///
/// It checks every memory read (Load) and memory write (Store) statement
/// to see whether any dynamically allocated or stack-allocated memory
/// location in the pointer's points-to set has been deallocated.
class UafChecker final : public Checker {
private:
  using PointsToSet = core::PointsToSet< MemoryLocation* >;

public:
  /// \brief Constructor
  explicit UafChecker(Context& ctx);

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
    ar::Value* operand; ///< The pointer operand being checked
    JsonDict info;
  };

  /// \brief Check a memory access (load or store) through the given pointer
  ///
  /// \param stmt   The statement performing the access
  /// \param pointer The pointer value being dereferenced
  /// \param is_write True if this is a store, false for a load
  /// \param inv    Abstract domain state at this point
  CheckResult check_mem_access(ar::Statement* stmt,
                               ar::Value* pointer,
                               bool is_write,
                               const value::AbstractDomain& inv);

  /// \brief Check whether accessing the given memory location is valid
  ///
  /// Returns:
  ///   Ok        — location is alive
  ///   Warning   — lifetime is unknown (possible UAF)
  ///   Error     — location is definitely deallocated (definite UAF)
  Result check_memory_location(ar::Statement* stmt,
                               ar::Value* pointer,
                               bool is_write,
                               const value::AbstractDomain& inv,
                               MemoryLocation* addr,
                               JsonDict& block_info);

  /// \brief Display a UAF check result, if display is requested
  std::optional< LogMessage > display_uaf_check(Result result,
                                                ar::Statement* stmt,
                                                ar::Value* pointer) const;

}; // end class UafChecker

} // end namespace analyzer
} // end namespace ikos
