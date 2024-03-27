#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Casting.h>
#include <map>
#include <set>
#include <string>
#include <vector>
using namespace llvm;

std::vector<std::pair<BasicBlock *, Value *>> defUseDep(Instruction *I) {
  std::vector<std::pair<BasicBlock *, Value *>> res;
  for (Instruction::const_op_iterator cuit = I->op_begin(); cuit != I->op_end();
       ++cuit) {
    if (Instruction *inst = dyn_cast<Instruction>(*cuit)) {
      if (Value *v = dyn_cast<Value>(cuit))
        res.push_back(std::pair(inst->getParent(), v));
    }
  }
  return res;
}

std::set<Value *> readVariables(Instruction *I) {
  std::set<Value *> res;
  if (isa<BranchInst>(I)) {
    return res;
  }
  for (auto &op : I->operands()) {
    if (isa<Instruction>(op)) {
      if (Value *v = dyn_cast<Value>(op))
        res.insert(v);
    }
  }
  return res;
}

std::set<Value *> writtenVariables(Instruction *I) {
  std::set<Value *> res;
  if (Value *v = dyn_cast<Value>(I)) {
    res.insert(v);
  }
  return res;
}

std::set<Value *> readVariables(BasicBlock *BB) {
  std::set<Value *> res;
  for (auto &I : *BB) {
    std::set<Value *> rv = readVariables(&I);
    res.insert(rv.begin(), rv.end());
  }
  return res;
}

std::set<Value *> writtenVariables(BasicBlock *BB) {
  std::set<Value *> res;
  for (auto &I : *BB) {
    std::set<Value *> wv = writtenVariables(&I);
    res.insert(wv.begin(), wv.end());
  }
  return res;
}

std::vector<std::pair<Instruction *, uint>>
RAWDep(Instruction *I,
       const std::map<std::string,
                      std::pair<std::set<StringRef>, std::set<StringRef>>>
           &bb_rw) {
  std::vector<std::pair<Instruction *, uint>> res;
  return res;
}

std::map<BasicBlock *, std::pair<std::set<Value *>, std::set<Value *>>>
get_BB_rw(Module *M) {
  std::map<BasicBlock *, std::pair<std::set<Value *>, std::set<Value *>>> res;
  for (auto &F : *M) {
    for (auto &BB : F) {
      std::set<Value *> rv = readVariables(&BB);
      std::set<Value *> wv = writtenVariables(&BB);
      // errs() << "BasicBlock no. of written variables" << wv.size() << "\n";
      // errs() << "BasicBlock no. of read variables" << rv.size() << "\n";
      res.insert({&BB, std::pair(rv, wv)});
    }
  }
  return res;
}

namespace {
struct DDGPass : public ModulePass {
  static char ID;
  DDGPass() : ModulePass(ID) {}

  bool runOnModule(Module &M) override {
    int bb_id = 0;
    std::map<BasicBlock *, std::pair<std::set<Value *>, std::set<Value *>>>
        bb_rw = get_BB_rw(&M);

    std::map<BasicBlock *, std::set<std::pair<BasicBlock *, Value *>>> DDG;
    for (auto &F : M) {
      for (auto &BB : F) {
        BB.setName(std::to_string(bb_id++));
      }
    }
    // errs() << "size of bb_rw: " << bb_rw.size() << "\n";
    for (auto &F : M) {
      for (auto &BB : F) {
        std::set<std::pair<BasicBlock *, Value *>> dep_set;
        for (BasicBlock *BB_child : successors(&BB)) {
          for (auto r : bb_rw.at(BB_child).first) {
            if (bb_rw.at(&BB).second.find(r) != bb_rw.at(&BB).second.end()) {
              if (&BB != BB_child)
                dep_set.insert(std::pair(BB_child, r));
            }
          }
        }
        for (auto &I : BB) {
          auto dep_vec = defUseDep(&I);
          for (auto &dep : dep_vec) {
            if (dep.second)
              if (&BB != dep.first)
                dep_set.insert(std::pair(dep.first, dep.second));
          }
        }
        DDG.insert({&BB, dep_set});
      }
    }

    for (auto const &[key, val] : DDG) {
      errs() << "\n" << key->getName() << "->";
      for (auto pairs : val) {
        errs() << "\t(" << pairs.first->getName() << ", " << *pairs.second
               << "),\n";
      }
    }
    return false;
  }
};
} // namespace
char DDGPass::ID = 0;
static RegisterPass<DDGPass> X("ddg-pass", "DDGPass Pass");
