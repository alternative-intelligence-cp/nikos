/*******************************************************************************
 *
 * ikos-pp -- LLVM bitcode Pre-Processor for Static Analysis
 *
 * Author: Jorge A. Navas
 *
 * Contributors: Maxime Arthaud
 *               Alternative Intelligence (NIKOS / LLVM 20 port)
 *
 * Contact: https://github.com/alternative-intelligence-cp/nikos
 *
 * Notices:
 *
 * Copyright (c) 2011-2019 United States Government as represented by the
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

#include <boost/filesystem.hpp>

#include <llvm/ADT/StringSet.h>
#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/InitializePasses.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/raw_ostream.h>

// Legacy pass creators still available in LLVM 20
#include <llvm/Transforms/Scalar.h>    // createDeadCodeEliminationPass, etc.
#include <llvm/Transforms/Utils.h>     // createLowerSwitchPass, etc.
#include <llvm/Transforms/InstCombine/InstCombine.h>   // createInstructionCombiningPass
#include <llvm/Transforms/IPO/AlwaysInliner.h>         // createAlwaysInlinerLegacyPass
#include <llvm/IR/IRPrintingPasses.h>                   // createPrintModulePass

// New-style passes for those removed from the legacy API in LLVM 20
#include <llvm/Transforms/IPO/GlobalDCE.h>
#include <llvm/Transforms/IPO/GlobalOpt.h>
#include <llvm/Transforms/IPO/Internalize.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/JumpThreading.h>
#include <llvm/Transforms/Scalar/LoopDeletion.h>
#include <llvm/Transforms/Scalar/SCCP.h>
#include <llvm/Transforms/Utils/UnifyFunctionExitNodes.h>

#include <ikos/core/support/assert.hpp>

#include <ikos/frontend/llvm/pass.hpp>

namespace ikos_pp = ikos::frontend::pass;

static llvm::cl::opt< std::string > InputFilename(
    llvm::cl::Positional,
    llvm::cl::desc("<input bitcode file>"),
    llvm::cl::Required,
    llvm::cl::value_desc("filename"));

static llvm::cl::opt< std::string > OutputFilename(
    "o",
    llvm::cl::desc("Override output filename"),
    llvm::cl::value_desc("filename"));

static llvm::cl::opt< bool > OutputAssembly(
    "S", llvm::cl::desc("Write output as LLVM assembly"));

static llvm::cl::list< std::string > EntryPoints(
    "entry-points",
    llvm::cl::desc("List of program entry points"),
    llvm::cl::CommaSeparated,
    llvm::cl::value_desc("function"));

static llvm::cl::opt< bool > InlineAll("inline-all",
                                       llvm::cl::desc("Inline all functions"));

static llvm::cl::opt< bool > NoVerify(
    "no-verify", llvm::cl::desc("Do not run the LLVM bitcode verifier"));

static llvm::cl::opt< bool > DiscardValueNames(
    "discard-value-names",
    llvm::cl::desc("Discard names from Value (other than GlobalValue)."),
    llvm::cl::init(false),
    llvm::cl::Hidden);

static llvm::cl::opt< bool > PreserveBitcodeUseListOrder(
    "preserve-bc-uselistorder",
    llvm::cl::desc("Preserve use-list order when writing LLVM bitcode."),
    llvm::cl::init(true),
    llvm::cl::Hidden);

static llvm::cl::opt< bool > PreserveAssemblyUseListOrder(
    "preserve-ll-uselistorder",
    llvm::cl::desc("Preserve use-list order when writing LLVM assembly."),
    llvm::cl::init(false),
    llvm::cl::Hidden);

enum OptLevelType { None, Basic, Aggressive };

static llvm::cl::opt< OptLevelType > OptLevel(
    "opt",
    llvm::cl::desc("Optimization level:"),
    llvm::cl::values(
        clEnumValN(None,
                   "none",
                   "Only passes required for the translation to AR"),
        clEnumValN(Basic, "basic", "Basic set of optimizations (recommended)"),
        clEnumValN(Aggressive,
                   "aggressive",
                   "Aggressive optimizations (not recommended)")),
    llvm::cl::init(Basic));

// ============================================================================
// Helper: Run new-style (PassInfoMixin) passes via PassBuilder
// ============================================================================

/// \brief Build and run a new PassManager pipeline for passes whose legacy
/// creators were removed in LLVM 20.
///
/// The remaining legacy passes (LowerSwitch, LowerAtomic, DCE, etc.) are
/// run via llvm::legacy::PassManager. This function handles only the passes
/// that MUST use the new API.
static void run_new_pm_passes(
    llvm::Module& module,
    OptLevelType opt_level,
    const llvm::StringSet<>& exclude_set,
    bool inline_all) {
  llvm::LoopAnalysisManager lam;
  llvm::FunctionAnalysisManager fam;
  llvm::CGSCCAnalysisManager cgam;
  llvm::ModuleAnalysisManager mam;

  llvm::PassBuilder pb;
  pb.registerModuleAnalyses(mam);
  pb.registerCGSCCAnalyses(cgam);
  pb.registerFunctionAnalyses(fam);
  pb.registerLoopAnalyses(lam);
  pb.crossRegisterProxies(lam, fam, cgam, mam);

  llvm::ModulePassManager mpm;

  if (opt_level == Aggressive) {
    // Internalize: mark non-entry functions as internal
    if (exclude_set.find("*") == exclude_set.end()) {
      mpm.addPass(llvm::InternalizePass(
          [&](const llvm::GlobalValue& gv) {
            return exclude_set.find(gv.getName()) != exclude_set.end();
          }));
    }

    // GlobalDCE — remove unused globals
    mpm.addPass(llvm::GlobalDCEPass());

    // GlobalOpt — global variable/function optimizations
    mpm.addPass(llvm::GlobalOptPass());

    // JumpThreading — (conditional) constant propagation
    mpm.addPass(llvm::createModuleToFunctionPassAdaptor(
        llvm::JumpThreadingPass()));

    // SCCP — sparse conditional constant propagation
    mpm.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::SCCPPass()));

    // LoopDeletion — remove dead loops
    mpm.addPass(llvm::createModuleToFunctionPassAdaptor(
        llvm::createFunctionToLoopPassAdaptor(llvm::LoopDeletionPass())));

    // Another round of GlobalDCE after optimizations
    mpm.addPass(llvm::GlobalDCEPass());
  } else if (opt_level == Basic) {
    // GlobalDCE — remove unused globals
    // note: unfortunately, it removes some debug info about global variables
    mpm.addPass(llvm::GlobalDCEPass());
  }

  // UnifyFunctionExitNodes — ensure one return per function (all opt levels)
  mpm.addPass(llvm::createModuleToFunctionPassAdaptor(
      llvm::UnifyFunctionExitNodesPass()));

  mpm.run(module, mam);
}

/// \brief Main for ikos-pp
int main(int argc, char** argv) {
  llvm::InitLLVM x(argc, argv);

  // Program name
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  std::string progname = boost::filesystem::path(argv[0]).filename().string();

  // Enable debug stream buffering
  llvm::EnableDebugBuffering = true;

  // Global context
  llvm::LLVMContext context;

  /*
   * Initialize and register passes still using the legacy API
   */

  llvm::PassRegistry& registry = *llvm::PassRegistry::getPassRegistry();
  llvm::initializeCore(registry);
  llvm::initializeScalarOpts(registry);
  llvm::initializeVectorization(registry);
  llvm::initializeIPO(registry);
  llvm::initializeAnalysis(registry);
  llvm::initializeTransformUtils(registry);
  llvm::initializeInstCombine(registry);
  llvm::initializeTarget(registry);
  llvm::initializeWinEHPreparePass(registry);
  llvm::initializeSafeStackLegacyPassPass(registry);
  llvm::initializeSjLjEHPreparePass(registry);
  llvm::initializeStackProtectorPass(registry);
  llvm::initializePreISelIntrinsicLoweringLegacyPassPass(registry);
  llvm::initializeGlobalMergePass(registry);
  llvm::initializeInterleavedLoadCombinePass(registry);
  llvm::initializeInterleavedAccessPass(registry);
  llvm::initializePostInlineEntryExitInstrumenterPass(registry);
  llvm::initializeUnreachableBlockElimLegacyPassPass(registry);
  llvm::initializeExpandReductionsPass(registry);
  llvm::initializeWasmEHPreparePass(registry);
  llvm::initializeWriteBitcodePassPass(registry);
  ikos_pp::initialize_ikos_passes(registry);

  /*
   * Parse parameters
   */

  const char* overview =
      "ikos-pp -- LLVM bitcode Pre-Processor for Static Analysis\n";
  llvm::cl::ParseCommandLineOptions(argc, argv, overview);

  // Error diagnostic
  llvm::SMDiagnostic err;

  // If -discard-value-names, discard all the names (except for GlobalValue)
  context.setDiscardValueNames(DiscardValueNames);

  // Load the input module
  std::unique_ptr< llvm::Module > module =
      llvm::parseIRFile(InputFilename, err, context);
  if (!module) {
    err.print(progname.c_str(), llvm::errs());
    return 1;
  }
  module->convertFromNewDbgValues();

  // Immediately run the verifier to catch any problems
  if (!NoVerify && verifyModule(*module, &llvm::errs())) {
    llvm::errs() << progname << ": " << InputFilename
                 << ": error: input module is broken!\n";
    return 1;
  }

  // Default to standard output
  if (OutputFilename.empty()) {
    OutputFilename = "-";
  }

  // Output stream
  std::error_code ec;
  std::unique_ptr< llvm::ToolOutputFile > output =
      std::make_unique< llvm::ToolOutputFile >(OutputFilename,
                                               ec,
                                               llvm::sys::fs::OF_None);
  if (ec) {
    llvm::errs() << progname << ": " << ec.message() << '\n';
    return 1;
  }

  /*
   * Build the pipeline.
   *
   * LLVM 20 Migration Notes:
   * ========================
   * Several legacy pass creators (createGlobalDCEPass, createJumpThreadingPass,
   * createSCCPPass, createLoopDeletionPass, createGlobalOptimizerPass,
   * createInternalizePass, createUnifyFunctionExitNodesPass) were removed in
   * LLVM 20. These passes are now only available via the new PassManager API
   * (PassInfoMixin<>). We use a hybrid approach:
   *
   *   Phase 1 — Legacy PassManager for passes that still have legacy creators
   *             AND for IKOS-specific passes (LowerSelect, LowerCstExpr, etc.)
   *             which are implemented as legacy FunctionPass.
   *
   *   Phase 2 — New PassManager for passes that lost their legacy creators
   *             (GlobalDCE, Internalize, JumpThreading, SCCP, LoopDeletion,
   *             GlobalOpt, UnifyFunctionExitNodes).
   */

  // Build entry-point exclusion set for Internalize
  llvm::StringSet<> exclude_set;
  if (EntryPoints.empty()) {
    exclude_set.insert("main");
  } else {
    for (const auto& entry_point : EntryPoints) {
      exclude_set.insert(entry_point);
    }
  }

  // ========================================================================
  // Phase 1: Legacy PassManager
  // ========================================================================
  {
    llvm::legacy::PassManager pass_manager;

    if (OptLevel == None) {
      // Remove switch constructions (opt -lowerswitch)
      pass_manager.add(llvm::createLowerSwitchPass());

      // Lower down atomic instructions (opt -loweratomic)
      pass_manager.add(llvm::createLowerAtomicPass());

      // Lower constant expressions to instructions (ikos-pp -lower-cst-expr)
      pass_manager.add(ikos_pp::create_lower_cst_expr_pass());

      // Lower down select instructions (ikos-pp -lower-select)
      pass_manager.add(ikos_pp::create_lower_select_pass());

    } else if (OptLevel == Basic) {
      // SSA (opt -mem2reg)
      pass_manager.add(llvm::createPromoteMemoryToRegisterPass());

      // Dead code elimination (opt -dce)
      pass_manager.add(llvm::createDeadCodeEliminationPass());

      // Remove switch constructions (opt -lowerswitch)
      pass_manager.add(llvm::createLowerSwitchPass());

      // Remove unreachable blocks also dead cycles
      pass_manager.add(ikos_pp::create_remove_unreachable_blocks_pass());

      // Lower down atomic instructions (opt -loweratomic)
      pass_manager.add(llvm::createLowerAtomicPass());

      // Lower constant expressions to instructions (ikos-pp -lower-cst-expr)
      pass_manager.add(ikos_pp::create_lower_cst_expr_pass());

      // Dead code elimination (opt -dce)
      pass_manager.add(llvm::createDeadCodeEliminationPass());

      // Lower down select instructions (ikos-pp -lower-select)
      pass_manager.add(ikos_pp::create_lower_select_pass());

    } else {
      ikos_assert(OptLevel == Aggressive);

      // Remove unreachable blocks
      pass_manager.add(ikos_pp::create_remove_unreachable_blocks_pass());

      // SSA (opt -mem2reg)
      pass_manager.add(llvm::createPromoteMemoryToRegisterPass());

      // Simplification (opt -simplifycfg)
      pass_manager.add(llvm::createCFGSimplificationPass());

      // Break aggregates (opt -sroa)
      pass_manager.add(llvm::createSROAPass());

      // Global value numbering and redundant load elimination (opt -gvn)
      // note: unfortunately, it removes some debug information
      pass_manager.add(llvm::createGVNPass());

      // Cleanup after breaking aggregates (opt -instcombine)
      // (bad for static analysis)
      pass_manager.add(llvm::createInstructionCombiningPass());

      // Simplification (opt -simplifycfg)
      pass_manager.add(llvm::createCFGSimplificationPass());

      // Dead code elimination (opt -dce)
      pass_manager.add(llvm::createDeadCodeEliminationPass());

      // Lower invoke's (opt -lowerinvoke)
      pass_manager.add(llvm::createLowerInvokePass());

      // Cleanup after lowering invoke's (opt -simplifycfg)
      pass_manager.add(llvm::createCFGSimplificationPass());

      if (InlineAll) {
        // Mark all functions always_inline (ikos-pp -mark-internal-inline)
        pass_manager.add(ikos_pp::create_mark_internal_inline_pass());

        // Inline always_inline functions (opt -always-inline)
        pass_manager.add(llvm::createAlwaysInlinerLegacyPass());
      }

      // Remove unreachable blocks
      pass_manager.add(ikos_pp::create_remove_unreachable_blocks_pass());

      // Dead code elimination (opt -dce)
      pass_manager.add(llvm::createDeadCodeEliminationPass());

      // Canonical form for loops (opt -loop-simplify)
      pass_manager.add(llvm::createLoopSimplifyPass());

      // Cleanup unnecessary blocks (opt -simplifycfg)
      pass_manager.add(llvm::createCFGSimplificationPass());

      // Loop-closed SSA (opt -lcssa)
      pass_manager.add(llvm::createLCSSAPass());

      // Loop invariant code motion (opt -licm)
      pass_manager.add(llvm::createLICMPass());

      // SSA (opt -mem2reg)
      pass_manager.add(llvm::createPromoteMemoryToRegisterPass());

      // Cleanup unnecessary blocks (opt -simplifycfg)
      pass_manager.add(llvm::createCFGSimplificationPass());

      // Dead code elimination (opt -dce)
      pass_manager.add(llvm::createDeadCodeEliminationPass());

      // Remove unreachable blocks also dead cycles
      pass_manager.add(ikos_pp::create_remove_unreachable_blocks_pass());

      // Remove switch constructions (opt -lowerswitch)
      pass_manager.add(llvm::createLowerSwitchPass());

      // Lower down atomic instructions (opt -loweratomic)
      pass_manager.add(llvm::createLowerAtomicPass());

      // Lower constant expressions to instructions (ikos-pp -lower-cst-expr)
      pass_manager.add(ikos_pp::create_lower_cst_expr_pass());

      // Dead code elimination (opt -dce)
      pass_manager.add(llvm::createDeadCodeEliminationPass());

      // After lowering constant expressions we remove all
      // side-effect-free printf-like functions. This can trigger the
      // removal of global strings that only feed them.
      pass_manager.add(ikos_pp::create_remove_printf_calls_pass());

      // Dead code elimination (opt -dce)
      pass_manager.add(llvm::createDeadCodeEliminationPass());

      // Lower down select instructions (ikos-pp -lower-select)
      pass_manager.add(ikos_pp::create_lower_select_pass());
    }

    // Run all legacy passes
    pass_manager.run(*module);
  }

  // ========================================================================
  // Phase 2: New PassManager (passes whose legacy creators were removed)
  // ========================================================================
  run_new_pm_passes(*module, OptLevel, exclude_set, InlineAll);

  // ========================================================================
  // Verification and output
  // ========================================================================

  // Check that the module is well formed on completion of optimization
  if (!NoVerify && verifyModule(*module, &llvm::errs())) {
    llvm::errs() << progname << ": error: output module is broken!\n";
    return 1;
  }

  // Output pass — write bitcode or assembly
  {
    llvm::legacy::PassManager output_pm;

    if (OutputAssembly) {
      output_pm.add(llvm::createPrintModulePass(output->os(),
                                                "",
                                                PreserveAssemblyUseListOrder));
    } else {
      output_pm.add(
          createBitcodeWriterPass(output->os(), PreserveBitcodeUseListOrder));
    }

    output_pm.run(*module);
  }

  output->keep();

  return 0;
}
