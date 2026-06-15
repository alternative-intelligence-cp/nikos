/*******************************************************************************
 *
 * \file
 * \brief Use-after-free checker implementation
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

#include <ikos/analyzer/checker/uaf.hpp>
#include <ikos/analyzer/analysis/literal.hpp>
#include <ikos/analyzer/json/helper.hpp>
#include <ikos/analyzer/support/cast.hpp>
#include <ikos/analyzer/util/log.hpp>
#include <ikos/ar/semantic/statement.hpp>

namespace ikos {
namespace analyzer {

UafChecker::UafChecker(Context& ctx) : Checker(ctx) {}

CheckerName UafChecker::name() const {
  return CheckerName::Uaf;
}

const char* UafChecker::description() const {
  return "Use-after-free and use-after-return analysis";
}

void UafChecker::check(ar::Statement* stmt,
                       const value::AbstractDomain& inv,
                       CallContext* call_context) {
  if (auto load = dyn_cast< ar::Load >(stmt)) {
    // Memory read: check pointer operand
    ar::Value* ptr_val = load->operand();
    if (!ptr_val->type()->is_pointer()) {
      return;
    }
    CheckResult r = this->check_mem_access(stmt, ptr_val, /*is_write=*/false, inv);
    this->display_invariant(r.result, stmt, inv);
    this->_checks.insert(r.kind,
                         CheckerName::Uaf,
                         r.result,
                         stmt,
                         call_context,
                         {r.operand},
                         r.info);
  } else if (auto store = dyn_cast< ar::Store >(stmt)) {
    // Memory write: check pointer operand
    ar::Value* ptr_val = store->pointer();
    CheckResult r = this->check_mem_access(stmt, ptr_val, /*is_write=*/true, inv);
    this->display_invariant(r.result, stmt, inv);
    this->_checks.insert(r.kind,
                         CheckerName::Uaf,
                         r.result,
                         stmt,
                         call_context,
                         {r.operand},
                         r.info);
  }
  // Other statement types (calls, arithmetic, etc.) are not pointer dereferences
  // and therefore are not relevant for UAF. The double-free checker handles free().
}

UafChecker::CheckResult UafChecker::check_mem_access(
    ar::Statement* stmt,
    ar::Value* pointer,
    bool is_write,
    const value::AbstractDomain& inv) {

  if (inv.is_normal_flow_bottom()) {
    if (auto msg = this->display_uaf_check(Result::Unreachable, stmt, pointer)) {
      *msg << "\n";
    }
    return {CheckKind::Unreachable, Result::Unreachable, pointer, {}};
  }

  const ScalarLit& ptr = this->_lit_factory.get_scalar(pointer);

  // Uninitialized pointer — not a UAF, flag as UVA
  if (ptr.is_undefined() ||
      (ptr.is_pointer_var() &&
       inv.normal().uninit_is_uninitialized(ptr.var()))) {
    // Not our concern — UninitializedVariableChecker handles this
    return {CheckKind::UninitializedVariable, Result::Ok, pointer, {}};
  }

  // Null pointer — not UAF (NullDereferenceChecker handles this)
  if (ptr.is_null() ||
      (ptr.is_pointer_var() && inv.normal().nullity_is_null(ptr.var()))) {
    return {CheckKind::NullPointerDereference, Result::Ok, pointer, {}};
  }

  if (!ptr.is_pointer_var()) {
    // Non-variable pointer (constant address etc.) — skip
    return {CheckKind::UseAfterFree, Result::Ok, pointer, {}};
  }

  // Collect points-to set
  PointsToSet addrs = inv.normal().pointer_to_points_to(ptr.var());

  if (addrs.is_empty()) {
    // Invalid pointer — not UAF specifically
    return {CheckKind::InvalidPointerDereference, Result::Ok, pointer, {}};
  } else if (addrs.is_top()) {
    // Unknown points-to — can't determine UAF safely, skip
    return {CheckKind::UseAfterFree, Result::Ok, pointer, {}};
  }

  // Per-address lifetime check
  bool all_error = true;
  bool all_ok = true;
  CheckKind worst_kind = CheckKind::UseAfterFree; // refined below
  bool has_uar = false; // set if any UseAfterReturn found

  JsonDict info;
  JsonList points_to_info;
  info.put("access_kind", to_json(is_write ? "write" : "read"));
  info.put("points_to_count", static_cast< int >(addrs.size()));

  for (MemoryLocation* addr : addrs) {
    JsonDict block_info = {
        {"id", this->_ctx.output_db->memory_locations.insert(addr)}};

    Result loc_result =
        this->check_memory_location(stmt, pointer, is_write, inv, addr,
                                    block_info);
    block_info.put("status", static_cast< int >(loc_result));

    // Track whether this location is a UAR vs UAF
    if (isa< LocalMemoryLocation >(addr)) {
      has_uar = true;
    }

    if (loc_result == Result::Error) {
      all_ok = false;
    } else if (loc_result == Result::Warning) {
      all_ok = false;
      all_error = false;
    } else {
      all_error = false;
    }

    points_to_info.add(block_info);
  }
  info.put("points_to", points_to_info);

  // Determine dominant kind: prefer UAR when any local location is involved
  worst_kind = has_uar ? CheckKind::UseAfterReturn : CheckKind::UseAfterFree;

  if (all_error) {
    if (auto msg = this->display_uaf_check(Result::Error, stmt, pointer)) {
      *msg << (worst_kind == CheckKind::UseAfterReturn
                   ? ": use after return\n"
                   : ": use after free\n");
    }
    return {worst_kind, Result::Error, pointer, std::move(info)};
  } else if (!all_ok) {
    if (auto msg = this->display_uaf_check(Result::Warning, stmt, pointer)) {
      *msg << ": possible use after free\n";
    }
    return {worst_kind, Result::Warning, pointer, std::move(info)};
  } else {
    return {CheckKind::UseAfterFree, Result::Ok, pointer, {}};
  }
}

Result UafChecker::check_memory_location(ar::Statement* stmt,
                                         ar::Value* pointer,
                                         bool is_write,
                                         const value::AbstractDomain& inv,
                                         MemoryLocation* addr,
                                         JsonDict& block_info) {
  ikos_ignore(stmt);
  ikos_ignore(pointer);
  ikos_ignore(is_write);

  if (isa< DynAllocMemoryLocation >(addr)) {
    // Heap-allocated memory — check lifetime
    auto lifetime = inv.normal().lifetime_to_lifetime(addr);

    if (lifetime.is_deallocated()) {
      // Definite use-after-free
      block_info.put("lifetime", to_json("deallocated"));
      return Result::Error;
    } else if (lifetime.is_top()) {
      // Possibly deallocated (ambiguous alias / conditional free)
      block_info.put("lifetime", to_json("maybe-deallocated"));
      return Result::Warning;
    } else {
      // Allocated — safe
      block_info.put("lifetime", to_json("allocated"));
      return Result::Ok;
    }
  } else if (isa< LocalMemoryLocation >(addr)) {
    // Stack-allocated memory — check for use-after-return
    auto lifetime = inv.normal().lifetime_to_lifetime(addr);

    if (lifetime.is_deallocated()) {
      // Definite use-after-return
      block_info.put("lifetime", to_json("deallocated"));
      block_info.put("kind", to_json("use-after-return"));
      return Result::Error;
    } else if (lifetime.is_top()) {
      block_info.put("lifetime", to_json("maybe-deallocated"));
      block_info.put("kind", to_json("use-after-return"));
      return Result::Warning;
    } else {
      block_info.put("lifetime", to_json("allocated"));
      return Result::Ok;
    }
  } else {
    // Global, function pointer, absolute zero, etc. — always safe for UAF
    return Result::Ok;
  }
}

std::optional< LogMessage > UafChecker::display_uaf_check(
    Result result, ar::Statement* stmt, ar::Value* pointer) const {
  auto msg = this->display_check(result, stmt);
  if (msg) {
    *msg << "check_uaf(";
    pointer->dump(msg->stream());
    *msg << ")";
  }
  return msg;
}

} // end namespace analyzer
} // end namespace ikos
