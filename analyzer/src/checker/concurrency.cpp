#include <ikos/analyzer/analysis/pointer/pointer.hpp>
#include <ikos/analyzer/checker/concurrency.hpp>
#include <ikos/analyzer/support/cast.hpp>
#include <ikos/analyzer/analysis/memory_location.hpp>

namespace ikos {
namespace analyzer {

ConcurrencyChecker::ConcurrencyChecker(Context& ctx) : Checker(ctx) {
  // Run an intraprocedural lockset analysis for each function
  for (auto it = _ctx.bundle->function_begin(), et = _ctx.bundle->function_end(); it != et; ++it) {
    ar::Function* fun = *it;
    if (fun->is_definition()) {
      analyze_function(fun);
    }
  }
}

CheckerName ConcurrencyChecker::name() const {
  return CheckerName::Concurrency;
}

const char* ConcurrencyChecker::description() const {
  return "Concurrency checker";
}

void ConcurrencyChecker::analyze_function(ar::Function* fun) {
  if (!fun->body()->has_entry_block()) {
    return;
  }

  boost::container::flat_map< ar::BasicBlock*, Lockset > bb_in;
  std::vector< ar::BasicBlock* > worklist;
  boost::container::flat_set< ar::BasicBlock* > in_worklist;

  auto push = [&](ar::BasicBlock* bb) {
    if (in_worklist.insert(bb).second) {
      worklist.push_back(bb);
    }
  };

  bb_in.emplace(fun->body()->entry_block(), Lockset::top());
  push(fun->body()->entry_block());

  while (!worklist.empty()) {
    ar::BasicBlock* bb = worklist.back();
    worklist.pop_back();
    in_worklist.erase(bb);

    Lockset current = bb_in.at(bb);

    for (ar::Statement* stmt : *bb) {
      auto lockset_it = _stmt_locksets.find(stmt);
      if (lockset_it != _stmt_locksets.end()) {
        lockset_it->second = current;
      } else {
        _stmt_locksets.emplace(stmt, current);
      }

      if (auto call = dyn_cast< ar::IntrinsicCall >(stmt)) {
        if (call->intrinsic_id() == ar::Intrinsic::LibcPthreadMutexLock) {
          if (auto gv = dyn_cast< ar::GlobalVariable >(call->argument(0))) {
            current.add(_ctx.mem_factory->get_global(gv));
          } else if (auto ptr = dyn_cast< ar::InternalVariable >(call->argument(0))) {
            const core::PointsToSet< MemoryLocation* >& pts = _ctx.pointer->results().get(_ctx.var_factory->get_internal(ptr)).points_to();
            for (MemoryLocation* mem : pts) {
              current.add(mem);
            }
          }
        } else if (call->intrinsic_id() == ar::Intrinsic::LibcPthreadMutexUnlock) {
          if (auto gv = dyn_cast< ar::GlobalVariable >(call->argument(0))) {
            current.remove(_ctx.mem_factory->get_global(gv));
          } else if (auto ptr = dyn_cast< ar::InternalVariable >(call->argument(0))) {
            const core::PointsToSet< MemoryLocation* >& pts = _ctx.pointer->results().get(_ctx.var_factory->get_internal(ptr)).points_to();
            for (MemoryLocation* mem : pts) {
              current.remove(mem);
            }
          }
        }
      }
    }

    // Propagate to successors
    for (auto it = bb->successor_begin(), et = bb->successor_end(); it != et; ++it) {
      ar::BasicBlock* succ = *it;
      auto in_it = bb_in.find(succ);
      if (in_it == bb_in.end()) {
        bb_in.emplace(succ, current);
        push(succ);
      } else {
        Lockset old_in = in_it->second;
        Lockset new_in = old_in;
        new_in.join_with(current); // Join is intersection!

        if (!new_in.equals(old_in)) {
          in_it->second = new_in;
          push(succ);
        }
      }
    }
  }
}

void ConcurrencyChecker::check(ar::Statement* stmt,
                               const value::AbstractDomain& inv,
                               CallContext* call_context) {
  if (inv.is_normal_flow_bottom()) {
    return; // Unreachable
  }

  if (auto load = dyn_cast< ar::Load >(stmt)) {
    check_memory_access(stmt, load->operand(), inv, call_context, false);
  } else if (auto store = dyn_cast< ar::Store >(stmt)) {
    check_memory_access(stmt, store->pointer(), inv, call_context, true);
  } else if (auto call = dyn_cast< ar::IntrinsicCall >(stmt)) {
    if (call->intrinsic_id() == ar::Intrinsic::LibcPthreadMutexLock) {
      // Deadlock check
      auto it = _stmt_locksets.find(stmt);
      if (it != _stmt_locksets.end() && !it->second.is_bottom()) {
        bool deadlock = false;
        if (auto gv = dyn_cast< ar::GlobalVariable >(call->argument(0))) {
          if (it->second.contains(_ctx.mem_factory->get_global(gv))) {
            deadlock = true;
          }
        } else if (auto ptr = dyn_cast< ar::InternalVariable >(call->argument(0))) {
          const core::PointsToSet< MemoryLocation* >& pts = _ctx.pointer->results().get(_ctx.var_factory->get_internal(ptr)).points_to();
          for (MemoryLocation* mem : pts) {
            if (it->second.contains(mem)) {
              deadlock = true;
              break;
            }
          }
        }

        if (deadlock) {
          _checks.insert(CheckKind::Deadlock,
                         CheckerName::Concurrency,
                         Result::Error,
                         stmt,
                         call_context,
                         {call->argument(0)});
          return;
        }
      }

      _checks.insert(CheckKind::Deadlock,
                     CheckerName::Concurrency,
                     Result::Ok,
                     stmt,
                     call_context,
                     {call->argument(0)});
    } else if (call->intrinsic_id() == ar::Intrinsic::LibcPthreadCreate ||
               call->intrinsic_id() == ar::Intrinsic::LibcPthreadJoin) {
      // Thread creation / join: emit an OK record so callers can enumerate
      // thread spawn / sync points from the database.
      _checks.insert(CheckKind::DataRace,
                     CheckerName::Concurrency,
                     Result::Ok,
                     stmt,
                     call_context);
    }
  }
}

void ConcurrencyChecker::check_memory_access(ar::Statement* stmt,
                                             ar::Value* pointer,
                                             const value::AbstractDomain& /*inv*/,
                                             CallContext* call_context,
                                             bool /*is_write*/) {
  auto it = _stmt_locksets.find(stmt);
  if (it == _stmt_locksets.end() || it->second.is_bottom()) {
    return;
  }
  const Lockset& lockset = it->second;

  core::PointsToSet< MemoryLocation* > pts = core::PointsToSet< MemoryLocation* >::empty();
  if (auto gv = dyn_cast< ar::GlobalVariable >(pointer)) {
    pts = core::PointsToSet< MemoryLocation* >{_ctx.mem_factory->get_global(gv)};
  } else if (auto ptr = dyn_cast< ar::InternalVariable >(pointer)) {
    pts = _ctx.pointer->results().get(_ctx.var_factory->get_internal(ptr)).points_to();
  } else {
    return;
  }
  bool accesses_shared = false;
  for (MemoryLocation* mem : pts) {
    if (isa< GlobalMemoryLocation >(mem) ||
        isa< DynAllocMemoryLocation >(mem)) {
      accesses_shared = true;
      break;
    }
  }

  if (accesses_shared) {
    if (lockset.empty()) {
      _checks.insert(CheckKind::DataRace,
                     CheckerName::Concurrency,
                     Result::Warning,
                     stmt,
                     call_context,
                     {pointer});
    } else {
      _checks.insert(CheckKind::DataRace,
                     CheckerName::Concurrency,
                     Result::Ok,
                     stmt,
                     call_context,
                     {pointer});
    }
  }
}

} // end namespace analyzer
} // end namespace ikos
