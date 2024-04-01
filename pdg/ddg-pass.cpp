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
#include <queue>
#include <set>
#include <string>
#include <vector>
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

std::pair<std::set<int>, std::set<int>>
get_secure_BB(std::map<int, std::set<std::pair<int, Value *>>> &DDG,
              std::set<std::pair<int, Value *>> classified, int n) {
  std::set<int> visited, secure, notSecure;
  visited.insert(0);
  for (auto c_pair : classified) {
    std::queue<int> q;
    q.push(c_pair.first);
    while (!q.empty()) {
      int curr = q.front();
      q.pop();
      for (auto neighbour : DDG.at(curr)) {
        errs() << "\nclassified:" << c_pair.first;
        if (visited.find(neighbour.first) == visited.end()) {
          if (classified.find(neighbour) != classified.end()) {
            secure.insert(neighbour.first);
          }
          visited.insert(neighbour.first);
          q.push(neighbour.first);
        }
      }
    }
  }
  return std::pair(secure, notSecure);
}

namespace {
struct DDGPass : public ModulePass {
  static char ID;
  DDGPass() : ModulePass(ID) {}

  bool runOnModule(Module &M) override {
    int bb_id = 0;
    std::map<BasicBlock *, std::pair<std::set<Value *>, std::set<Value *>>>
        bb_rw = get_BB_rw(&M);
    std::set<Value *> secure_vars = getSecureVars(&M);
    std::set<std::pair<int, Value *>> classified_BB;
    std::set<int> secure_BB;
    std::set<int> insecure_BB;

    std::map<int, std::set<std::pair<int, Value *>>> DDG;
    for (auto &F : M) {
      for (auto &BB : F) {
        BB.setName(std::to_string(bb_id++));
        std::set<std::pair<int, Value *>> dep_set;
        DDG.insert({std::stoi(BB.getName().str()), dep_set});
        errs() << "BB" << bb_id << "\n------------------------------------"
               << BB << "------------------------------------------\n";
      }
    }
    // errs() << "size of bb_rw: " << bb_rw.size() << "\n";
    for (auto &F : M) {
      for (auto &BB : F) {
        // errs() << "\n" << std::stoi(BB.getName().str()) << "->";
        for (BasicBlock *BB_child : successors(&BB)) {
          for (auto r : bb_rw.at(BB_child).first) {
            if (bb_rw.at(&BB).second.find(r) != bb_rw.at(&BB).second.end()) {
              if (&BB != BB_child) {
                // errs() << "\t(" << std::stoi(BB_child->getName().str()) << ",
                // "
                //        << *r << ", RAWDep\n";
                DDG.at(std::stoi(BB_child->getName().str()))
                    .insert(std::pair(std::stoi(BB.getName().str()), r));
              }
            }
          }
        }
        for (auto &I : BB) {
          if (Value *val = dyn_cast<Value>(&I)) {
            if (secure_vars.find(val) != secure_vars.end()) {
              classified_BB.insert(
                  std::pair(std::stoi(BB.getName().str()), val));
              secure_BB.insert(std::stoi(BB.getName().str()));
            }
          }
          auto dep_vec = defUseDep(&I);
          for (auto &dep : dep_vec) {
            if (dep.second)
              if (&BB != dep.first) {

                // errs() << "\t(" << std::stoi(dep.first->getName().str()) <<
                // ", "
                //        << *dep.second << ", DefUseDep\n";
                DDG.at(std::stoi(BB.getName().str()))
                    .insert(std::pair(std::stoi(dep.first->getName().str()),
                                      dep.second));
              }
          }
        }
      }
    }

    std::pair<std::set<int>, std::set<int>> sns =
        get_secure_BB(DDG, classified_BB, bb_id);

    errs() << "\n Secure: ";
    for (auto it : sns.first) {
      errs() << it << ", ";
    }
    errs() << "\n Not Secure: ";
    for (auto it : sns.second) {
      errs() << it << ", ";
    }

    for (auto const &[key, val] : DDG) {
      errs() << "\n" << key << "->";
      for (auto pairs : val) {
        errs() << "\t(" << pairs.first << ", " << *pairs.second << "),\n";
      }
    }
    return false;
  }
};
} // namespace
char DDGPass::ID = 0;
static RegisterPass<DDGPass> X("ddg-pass", "DDGPass Pass");
