#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
namespace {
  struct CustomModulePass : public ModulePass {
    static char ID;
    CustomModulePass() : ModulePass(ID) {}

    bool runOnModule(Module &M) override {
      int bb_id = 0;
      for (auto& F : M) {
        for (auto& BB : F) {
          errs() << "Basic block"<< bb_id++ << "\n" << BB << "\n";
          for(auto* pred: predecessors(&BB)) {
          errs() << "predecessor: "<< *pred << "\n";
          }
        }
        }
      return false;
      }
    };
}
char CustomModulePass::ID = 0;
static RegisterPass<CustomModulePass> X("custom-module-pass", "CustomModulePass Pass");
