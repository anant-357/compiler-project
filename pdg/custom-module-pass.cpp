#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/IR/Instruction.h>
#include <llvm/Support/Casting.h>
#include <map>
#include <set>

using namespace llvm;
namespace {
struct CustomModulePass : public ModulePass {
  static char ID;
  CustomModulePass() : ModulePass(ID) {}

  bool runOnModule(Module &M) override {
    int bb_id = 0;
    std::map<int, std::set<int>> DDG;
    for (auto &F : M) {
      for (auto &BB : F) {
        std::set<Value *> readVars, writtenVars;
        for (auto &I : BB) {
          for (int i = 0; i < I.getNumOperands(); i++) {
            Value *op = I.getOperand(i);
            if (isa<Instruction>(op) && readVars.count(op)) {
              DDG[bb_id].insert(
                  cast<Instruction>(op)->getParent()->getName().str().c_str());
            }
            readVars.insert(op);

            if (StoreInst *store = dyn_cast<StoreInst>(&I)) {
              Value *writtenVal = store->getValueOperand();
              if (isa<Instruction>(writtenVal) &&
                  writtenVars.count(writtenVal)) {
                DDG[bb_id].insert(cast<Instruction>(writtenVal)
                                      ->getParent()
                                      ->getName()
                                      .str()
                                      .c_str());
              }
              writtenVars.insert(writtenVal);
            }
          }
        }
      }
      bb_id++;
    }
    return false;
  }
};
} // namespace
char CustomModulePass::ID = 0;
static RegisterPass<CustomModulePass> X("custom-module-pass",
                                        "CustomModulePass Pass");
