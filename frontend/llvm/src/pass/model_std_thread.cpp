/*******************************************************************************
 *
 * \file
 * \brief Implementation of the ModelStdThreadPass
 *
 * Author: NIKOS 2.1 Developer
 *
 ******************************************************************************/

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include <ikos/frontend/llvm/pass.hpp>

#define DEBUG_TYPE "model-std-thread"

using namespace llvm;

namespace {

/// \brief Model std::thread pass
///
/// This pass intercepts calls to std::thread::_State_impl constructors, finds
/// the corresponding virtual _M_run method, and inserts a call to 
/// __ikos_pthread_create_wrapper with the _M_run function and the state object.
struct ModelStdThreadPass final : public ModulePass {
  static char ID;

  ModelStdThreadPass() : ModulePass(ID) {}

  void getAnalysisUsage(AnalysisUsage&) const override {
    // All analyses are invalidated
  }

  bool runOnModule(Module& M) override {
    bool changed = false;

    // We will look for calls to the _State_impl constructor.
    // Example: _ZNSt6thread11_State_implINS_8_InvokerISt5tupleIJZ1fvE3$_0EEEEEC2IJS3_EEEDpOT_
    
    // First, declare our wrapper if it's not already in the module
    // void __ikos_pthread_create_wrapper(void* run_fn, void* arg);
    Type* void_ptr_ty = PointerType::getUnqual(M.getContext());
    FunctionType* wrapper_ty = FunctionType::get(
        Type::getVoidTy(M.getContext()), {void_ptr_ty, void_ptr_ty}, false);
    FunctionCallee wrapper_callee = M.getOrInsertFunction("__ikos_pthread_create_wrapper", wrapper_ty);

    for (Function& F : M) {
      if (F.isDeclaration()) continue;

      std::vector<Instruction*> to_remove;
      for (auto it = inst_begin(F), et = inst_end(F); it != et; ++it) {
        Instruction* inst = &*it;
        if (auto* call = dyn_cast<CallBase>(inst)) {
          if (Function* callee = call->getCalledFunction()) {
            StringRef name = callee->getName();
            llvm::errs() << "ModelStdThreadPass visiting: " << name << "\n";
            if (name.starts_with("_ZNSt6thread11_State_impl") && (name.contains("EC1") || name.contains("EC2"))) {
              // Extract the class prefix
              size_t idx = name.find("EC1");
              if (idx == StringRef::npos) {
                idx = name.find("EC2");
              }
              
              if (idx != StringRef::npos) {
                StringRef prefix = name.substr(0, idx + 1);
                std::string run_fn_name = prefix.str() + "6_M_runEv";
                
                if (Function* run_fn = M.getFunction(run_fn_name)) {
                  // We found the thread entry function!
                  // Insert a call to __ikos_pthread_create_wrapper right after this instruction.
                  // The state pointer is the first argument (index 0) passed to the constructor.
                  Value* state_ptr = call->getArgOperand(0);
                  
                  IRBuilder<> builder(call->getNextNode());
                  Value* casted_run_fn = builder.CreateBitCast(run_fn, void_ptr_ty);
                  Value* casted_state_ptr = builder.CreateBitCast(state_ptr, void_ptr_ty);
                  
                  builder.CreateCall(wrapper_callee, {casted_run_fn, casted_state_ptr});
                  changed = true;
                }
              }
            } else if (name == "pthread_once" || name == "__gthread_once") {
              // pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
              // We will replace the call to pthread_once with a direct call to init_routine
              if (call->arg_size() >= 2) {
                Value* init_routine = call->getArgOperand(1);
                
                // init_routine has type void (*)(void)
                FunctionType* init_ty = FunctionType::get(Type::getVoidTy(M.getContext()), false);
                IRBuilder<> builder(call);
                
                // Directly call the init_routine
                builder.CreateCall(init_ty, init_routine);
                
                // Since pthread_once returns int, we replace its result with 0 (success)
                call->replaceAllUsesWith(ConstantInt::get(Type::getInt32Ty(M.getContext()), 0));
                
                // Remove the pthread_once call
                to_remove.push_back(call);
                changed = true;
              }
            }
          }
        }
      }
      
      for (Instruction* inst : to_remove) {
        inst->eraseFromParent();
      }
    }

    return changed;
  }
};

} // end anonymous namespace

char ModelStdThreadPass::ID = 0;

INITIALIZE_PASS(ModelStdThreadPass,
                "model-std-thread",
                "Model std::thread spawns",
                false,
                false)

llvm::ModulePass* ikos::frontend::pass::create_model_std_thread_pass() {
  return new ModelStdThreadPass();
}
