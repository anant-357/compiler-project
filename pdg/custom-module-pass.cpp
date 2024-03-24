#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
namespace {
  struct InstructionCounter : public ModulePass {
    static char ID;
    InstructionCounter() : ModulePass(ID) {}

    bool runOnModule(Module &M) override {
      int bb_id = 0;
      for (auto& F : M) {
        for (auto& BB : F) {
          errs() << "Basic block"<< bb_id++ << "\n" << BB << "\n";
        }
        }
      return false;
      }
    };
}
char InstructionCounter::ID = 0;
static RegisterPass<InstructionCounter> X("instruction-counter", "InstructionCounter Pass");
