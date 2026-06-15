#include <set>
#include <string>

#include <ikos/analyzer/checker/taint.hpp>
#include <ikos/analyzer/analysis/literal.hpp>
#include <ikos/analyzer/json/helper.hpp>
#include <ikos/analyzer/support/cast.hpp>
#include <ikos/analyzer/util/log.hpp>
#include <ikos/ar/semantic/code.hpp>

namespace ikos {
namespace analyzer {

TaintChecker::TaintChecker(Context& ctx) : Checker(ctx) {}

CheckerName TaintChecker::name() const {
  return CheckerName::Taint;
}

const char* TaintChecker::description() const {
  return "Taint analysis checker";
}

/// \brief Strip AR intrinsic prefixes (ar.libc., ar.libcpp., ar.ikos.) so that
/// taint config entries using standard library names (e.g., "printf") match
/// the AR-mangled intrinsic names (e.g., "ar.libc.printf").
static std::string normalize_callee_name(const std::string& name) {
  static const char* prefixes[] = {
      "ar.libc.", "ar.libcpp.", "ar.ikos.", "ar.intrinsic.", nullptr};
  for (const char** p = prefixes; *p; ++p) {
    std::size_t plen = std::strlen(*p);
    if (name.size() > plen && name.compare(0, plen, *p) == 0) {
      return name.substr(plen);
    }
  }
  return name;
}

/// \brief Try to resolve a CallBase's callee to a concrete ar::Function*.
///
/// Handles two cases:
///   1. Direct call:  called() is a FunctionPointerConstant  →  trivial.
///   2. Bitcast call: called() is an InternalVariable defined by a
///      UnaryOperation(Bitcast, FunctionPointerConstant).  This pattern
///      appears when the frontend emits variadic intrinsic calls (e.g.,
///      printf, fopen) as a bitcast of the function pointer followed by a
///      regular call through the temporary.
///
/// Returns nullptr if the callee cannot be resolved statically.
static ar::Function* try_resolve_callee_function(ar::CallBase* call) {
  ar::Value* called_op = call->called();

  // Case 1: direct call to a known function pointer constant
  if (auto called_fn = dyn_cast< ar::FunctionPointerConstant >(called_op)) {
    return called_fn->function();
  }

  // Case 2: call through a bitcast temporary (common for variadic intrinsics)
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

void TaintChecker::check(ar::Statement* stmt,
                         const value::AbstractDomain& inv,
                         CallContext* call_context) {
  if (!this->_ctx.taint_config) {
    return;
  }

  if (auto call = dyn_cast< ar::CallBase >(stmt)) {
    ar::Function* fun = try_resolve_callee_function(call);
    if (!fun) {
      return;
    }
    // Normalize: strip ar.libc.* prefixes so config entries like "printf"
    // match the AR intrinsic name "ar.libc.printf".
    std::string lookup_name = normalize_callee_name(fun->name());
    if (auto sink = this->_ctx.taint_config->get_sink(lookup_name)) {
      CheckResult check = this->check_call(call, inv, sink);
      this->display_invariant(check.result, stmt, inv);
      std::vector< ar::Value* > call_args;
      for (unsigned i = 0; i < call->num_arguments(); ++i) {
        call_args.push_back(call->argument(i));
      }
      this->_checks.insert(check.kind,
                           CheckerName::Taint,
                           check.result,
                           stmt,
                           call_context,
                           call_args,
                           check.info);
    }
  }
}

TaintChecker::CheckResult TaintChecker::check_call(
    ar::CallBase* call,
    const value::AbstractDomain& inv,
    const TaintConfig::Sink* sink) {

  if (inv.is_normal_flow_bottom()) {
    if (auto msg = this->display_taint_check(Result::Unreachable, call)) {
      *msg << "\n";
    }
    return {CheckKind::Unreachable, Result::Unreachable, {}};
  }

  bool is_tainted = false;
  std::set< std::string > all_labels; // union of provenance labels

  for (unsigned idx : sink->sensitive_args) {
    if (idx < call->num_arguments()) {
      ar::Value* arg = call->argument(idx);
      const ScalarLit& lit = this->_lit_factory.get_scalar(arg);

      if (lit.is_var()) {
        Variable* var = lit.var();
        if (inv.normal().taint_is_tainted(var)) {
          is_tainted = true;
          // Collect provenance labels
          for (const auto& lbl : inv.normal().taint_get_labels(var)) {
            all_labels.insert(lbl);
          }
        } else if (inv.normal().taint_is_memory_tainted(var)) {
          is_tainted = true;
          // Memory-tainted (e.g., fgets buffer) — label is the memory source
          all_labels.insert("(memory)");
        }
      }
    }
  }

  CheckKind kind = CheckKind::CommandInjection;
  switch (sink->type) {
    case TaintConfig::SinkType::CommandInjection:
      kind = CheckKind::CommandInjection;
      break;
    case TaintConfig::SinkType::PathTraversal:
      kind = CheckKind::PathTraversal;
      break;
    case TaintConfig::SinkType::FormatString:
      kind = CheckKind::FormatString;
      break;
    case TaintConfig::SinkType::SQLInjection:
      kind = CheckKind::SQLInjection;
      break;
    default:
      kind = CheckKind::CommandInjection;
      break;
  }

  if (is_tainted) {
    // Build provenance string for display
    std::string provenance;
    if (!all_labels.empty()) {
      provenance = " (tainted by: ";
      bool first = true;
      for (const auto& lbl : all_labels) {
        if (!first) provenance += ", ";
        provenance += lbl;
        first = false;
      }
      provenance += ")";
    }

    if (auto msg = this->display_taint_check(Result::Error, call)) {
      *msg << ": tainted value flows into sensitive sink" << provenance << "\n";
    }

    // Build info dict with provenance labels for the output database
    JsonDict info;
    if (!all_labels.empty()) {
      JsonList labels_json;
      for (const auto& lbl : all_labels) {
        labels_json.add(to_json(lbl));
      }
      info.put("taint_sources", labels_json);
    }
    return {kind, Result::Error, std::move(info)};
  } else {
    if (auto msg = this->display_taint_check(Result::Ok, call)) {
      *msg << ": safe\n";
    }
    return {kind, Result::Ok, {}};
  }
}

std::optional< LogMessage > TaintChecker::display_taint_check(
    Result result, ar::CallBase* stmt) const {
  auto msg = this->display_check(result, stmt);
  if (msg) {
    *msg << "check_taint(";
    stmt->dump(msg->stream());
    *msg << ")";
  }
  return msg;
}

} // end namespace analyzer
} // end namespace ikos
