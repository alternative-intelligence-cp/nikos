/*******************************************************************************
 *
 * \file
 * \brief Use-after-move checker implementation
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

#include <iostream>
#include <ikos/analyzer/checker/move_semantics.hpp>
#include <ikos/analyzer/analysis/literal.hpp>
#include <ikos/analyzer/json/helper.hpp>
#include <ikos/analyzer/support/cast.hpp>
#include <ikos/analyzer/util/log.hpp>
#include <ikos/ar/semantic/intrinsic.hpp>
#include <ikos/ar/semantic/statement.hpp>

namespace ikos {
namespace analyzer {

MoveChecker::MoveChecker(Context& ctx) : Checker(ctx) {}

CheckerName MoveChecker::name() const {
  return CheckerName::Uam;
}

const char* MoveChecker::description() const {
  return "Use-after-move analysis for std::unique_ptr and move-only types";
}

/// \brief Try to resolve a CallBase's callee to a concrete ar::Function*.
static ar::Function* try_resolve_callee_function(ar::CallBase* call) {
  ar::Value* called_op = call->called();

  // Case 1: direct call to a known function pointer constant
  if (auto called_fn = dyn_cast< ar::FunctionPointerConstant >(called_op)) {
    return called_fn->function();
  }

  // Case 2: call through a bitcast temporary
  if (auto iv = dyn_cast< ar::InternalVariable >(called_op)) {
    // Walk backward through the parent basic block to find the defining stmt
    ar::BasicBlock* bb = call->parent();
    for (auto it = bb->rbegin(), et = bb->rend(); it != et; ++it) {
      ar::Statement* s = *it;
      if (s == call) continue;
      if (!s->has_result()) continue;
      if (s->result() != iv) continue;
      // Found the defining statement — check if it's a Bitcast
      if (auto unary = dyn_cast< ar::UnaryOperation >(s)) {
        if (unary->op() == ar::UnaryOperation::Bitcast) {
          if (auto fn_cst =
                  dyn_cast< ar::FunctionPointerConstant >(unary->operand())) {
            return fn_cst->function();
          }
        }
      }
      break; // defined by something else, stop
    }
  }

  return nullptr;
}

void MoveChecker::check(ar::Statement* stmt,
                        const value::AbstractDomain& inv,
                        CallContext* call_context) {
  if (auto call = dyn_cast< ar::CallBase >(stmt)) {
    if (ar::Function* callee = try_resolve_callee_function(call)) {
      llvm::StringRef name = callee->name();
      
      // Check if it's a move constructor or move assignment
      if ((callee->is_intrinsic() &&
           callee->intrinsic_id() == ar::Intrinsic::LibcppUniquePtrMove) ||
          ((name.starts_with("_ZNSt10unique_ptr") || name.starts_with("_ZN8MoveOnly")) &&
           (name.contains("C2EOS") || name.contains("C1EOS") ||
            name.contains("aSEOS")))) {
        // This is a move — record the source as moved-from
        this->record_move(call, inv);
      } 
      // Check if it's a use of moved object (operator*, operator->, get, etc.)
      else if (name.starts_with("_ZNKSt10unique_ptr") || 
               name.starts_with("_ZNSt10unique_ptr") ||
               name.starts_with("_ZNK8MoveOnly") ||
               name.starts_with("_ZN8MoveOnly")) {
        // Exclude destructors and constructors
        if (!name.contains("D1Ev") && !name.contains("D2Ev") && 
            !name.contains("C1E") && !name.contains("C2E")) {
          // It's a method call. Argument 0 is `this`.
          if (call->num_arguments() > 0 && call->argument(0)->type()->is_pointer()) {
            CheckResult r = this->check_mem_access(call, call->argument(0), /*is_write=*/false, inv);
            if (r.result != Result::Ok) {
              this->display_invariant(r.result, call, inv);
              this->_checks.insert(r.kind,
                                   CheckerName::Uam,
                                   r.result,
                                   call,
                                   call_context,
                                   {r.operand},
                                   r.info);
            }
          }
        }
      }
    }
  }

  if (auto load = dyn_cast< ar::Load >(stmt)) {
    ar::Value* ptr_val = load->operand();
    if (!ptr_val->type()->is_pointer()) {
      return;
    }
    CheckResult r =
        this->check_mem_access(stmt, ptr_val, /*is_write=*/false, inv);
    if (r.result != Result::Ok) {
      this->display_invariant(r.result, stmt, inv);
      this->_checks.insert(r.kind,
                           CheckerName::Uam,
                           r.result,
                           stmt,
                           call_context,
                           {r.operand},
                           r.info);
    }
  } else if (auto store = dyn_cast< ar::Store >(stmt)) {
    ar::Value* ptr_val = store->pointer();
    CheckResult r =
        this->check_mem_access(stmt, ptr_val, /*is_write=*/true, inv);
    if (r.result != Result::Ok) {
      this->display_invariant(r.result, stmt, inv);
      this->_checks.insert(r.kind,
                           CheckerName::Uam,
                           r.result,
                           stmt,
                           call_context,
                           {r.operand},
                           r.info);
    }
  }
}

void MoveChecker::record_move(ar::CallBase* call,
                              const value::AbstractDomain& inv) {
  if (inv.is_normal_flow_bottom()) {
    std::cerr << "UAM Debug: normal flow is bottom\n";
    return;
  }
  if (call->num_arguments() < 2) {
    std::cerr << "UAM Debug: num_arguments < 2\n";
    return; // Unexpected signature — skip
  }

  // Argument 1 is the *source* unique_ptr (the one being moved from).
  // It is a pointer to the unique_ptr struct on the stack.
  ar::Value* src_arg = call->argument(1);
  if (!src_arg->type()->is_pointer()) {
    std::cerr << "UAM Debug: argument 1 is not a pointer\n";
    return;
  }

  const ScalarLit& src_lit = this->_lit_factory.get_scalar(src_arg);
  if (!src_lit.is_pointer_var()) {
    std::cerr << "UAM Debug: src_lit is not a pointer var\n";
    return;
  }

  // Collect all memory locations this pointer can reach
  PointsToSet addrs =
      inv.normal().pointer_to_points_to(src_lit.var());

  if (addrs.is_bottom() || addrs.is_top()) {
    std::cerr << "UAM Debug: addrs is bottom or top (is_bottom=" << addrs.is_bottom() << ", is_top=" << addrs.is_top() << ")\n";
    return; // Unknown source — be conservative, don't record
  }

  for (MemoryLocation* addr : addrs) {
    std::cerr << "UAM: Recording moved-from memory location: ";
    addr->dump(std::cerr);
    std::cerr << "\n";
    _moved_from.insert(addr);
  }
}

MoveChecker::CheckResult MoveChecker::check_mem_access(
    ar::Statement* stmt,
    ar::Value* pointer,
    bool is_write,
    const value::AbstractDomain& inv) {

  if (inv.is_normal_flow_bottom()) {
    if (auto msg = this->display_uam_check(Result::Unreachable, stmt, pointer)) {
      *msg << "\n";
    }
    return {CheckKind::Unreachable, Result::Unreachable, pointer, {}};
  }

  if (_moved_from.empty()) {
    return {CheckKind::UseAfterMove, Result::Ok, pointer, {}};
  }

  // Skip checks if we are currently inside a move constructor or move assignment
  // otherwise we will flag the legitimate move operations (stealing resources)
  if (ar::Code* code = stmt->code()) {
    if (ar::Function* current_func = code->function_or_null()) {
      llvm::StringRef func_name = current_func->name();
      if (func_name.contains("C2EOS") || func_name.contains("C1EOS") ||
          func_name.contains("aSEOS") || func_name.contains("C2E") ||
          func_name.contains("D1Ev") || func_name.contains("D2Ev")) {
        return {CheckKind::UseAfterMove, Result::Ok, pointer, {}};
      }
    }
  }

  const ScalarLit& ptr = this->_lit_factory.get_scalar(pointer);

  if (ptr.is_undefined() ||
      (ptr.is_pointer_var() &&
       inv.normal().uninit_is_uninitialized(ptr.var()))) {
    return {CheckKind::UninitializedVariable, Result::Ok, pointer, {}};
  }

  if (ptr.is_null() ||
      (ptr.is_pointer_var() && inv.normal().nullity_is_null(ptr.var()))) {
    // Null deref — handled by NullDereferenceChecker
    return {CheckKind::UseAfterMove, Result::Ok, pointer, {}};
  }

  if (!ptr.is_pointer_var()) {
    return {CheckKind::UseAfterMove, Result::Ok, pointer, {}};
  }

  PointsToSet addrs = inv.normal().pointer_to_points_to(ptr.var());
  if (addrs.is_bottom() || addrs.is_top()) {
    return {CheckKind::UseAfterMove, Result::Ok, pointer, {}};
  }

  std::cerr << "UAM: check_mem_access processing ";
  pointer->dump(std::cerr);
  std::cerr << " with points-to:\n";
  for (MemoryLocation* addr : addrs) {
    std::cerr << "  - ";
    addr->dump(std::cerr);
    std::cerr << "\n";
  }

  // Check if any address in the points-to set is a moved-from location
  bool any_moved = false;
  bool all_moved = true;

  JsonDict info;
  JsonList pts_info;
  info.put("access_kind", to_json(is_write ? "write" : "read"));

  for (MemoryLocation* addr : addrs) {
    JsonDict block;
    block.put("id", this->_ctx.output_db->memory_locations.insert(addr));
    bool moved = (_moved_from.count(addr) > 0);
    block.put("moved_from", moved);
    pts_info.add(block);

    if (moved) {
      any_moved = true;
    } else {
      all_moved = false;
    }
  }
  info.put("points_to", pts_info);
  info.put("points_to_count", static_cast< int >(addrs.size()));

  if (!any_moved) {
    return {CheckKind::UseAfterMove, Result::Ok, pointer, {}};
  }

  if (all_moved) {
    if (auto msg = this->display_uam_check(Result::Error, stmt, pointer)) {
      *msg << ": use after move\n";
    }
    return {CheckKind::UseAfterMove, Result::Error, pointer,
            std::move(info)};
  } else {
    // Some but not all locations are moved-from → possible UAM
    if (auto msg = this->display_uam_check(Result::Warning, stmt, pointer)) {
      *msg << ": possible use after move\n";
    }
    return {CheckKind::UseAfterMove, Result::Warning, pointer,
            std::move(info)};
  }
}

std::optional< LogMessage > MoveChecker::display_uam_check(
    Result result, ar::Statement* stmt, ar::Value* pointer) const {
  auto msg = this->display_check(result, stmt);
  if (msg) {
    *msg << "check_uam(";
    pointer->dump(msg->stream());
    *msg << ")";
  }
  return msg;
}

} // end namespace analyzer
} // end namespace ikos
