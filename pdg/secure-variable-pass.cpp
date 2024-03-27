#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/Casting.h>
#include <set>

using namespace llvm;

Value *getStoringArg(Instruction *I) {
  if (auto *ci = dyn_cast<CallInst>(I)) {
    for (Value *arg : ci->args()) {
      if (!isa<GlobalVariable>(arg) && !isa<Argument>(arg)) {
        return arg;
      }
    }
  }
  return 0;
}

bool checkUserInput(Instruction *I) {
  if (auto *ci = dyn_cast<CallInst>(I)) {
    for (Value *arg : ci->args()) {
      if (arg->getName().contains("cin")) {
        return true;
      }
    }
  }
  return false;
}

std::set<Value *> getSecureVars(Module *M) {
  std::set<Value *> secure;
  for (auto &F : *M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (checkUserInput(&I)) {
          secure.insert(getStoringArg(&I));
        }
      }
    }
  }
  return secure;
}

namespace {
struct SecureVariablePass : public ModulePass {
  static char ID;
  SecureVariablePass() : ModulePass(ID) {}

  bool runOnModule(Module &M) override {
    std::set<Value *> secure = getSecureVars(&M);
    for (auto &var : secure) {
      errs() << *var << "\n";
    }
    return false;
  }
};
} // namespace
char SecureVariablePass::ID = 0;
static RegisterPass<SecureVariablePass> X("secure-variable-pass",
                                          "SecureVariablePass Pass");
