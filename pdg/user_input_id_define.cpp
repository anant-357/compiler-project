#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
  struct user_input_id_define : public ModulePass {
    static char ID;
    user_input_id_define() : ModulePass(ID) {}

    bool runOnModule(Module &M) override {
      for (auto& F : M) {
        for (auto& BB : F) {
          for (auto& I : BB) {
            if (auto *callInst = dyn_cast<CallInst>(&I)) {
              Function *calledFunc = callInst->getCalledFunction();
                errs() << "User input detected in function: " << calledFunc->getName() << "\n";
              if (calledFunc && (calledFunc->getName().contains("scanf") || calledFunc->getName().contains("cin"))) {
                errs() << "User input detected in function: " << F.getName() << ", basic block: " << BB.getName() << "\n";
                errs() << "Instruction: " << I << "\n";
              }
            }
          }
        }
      }
      return false;
    }
  };
}

char user_input_id_define::ID = 0;
static RegisterPass<user_input_id_define> X("user_input_id_define", "user_input_id_define Pass");
