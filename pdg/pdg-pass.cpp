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
void get_secure_BB_CDG(std::map<int, std::vector<int>> &CDG,
                       std::set<std::pair<int, Value *>> ids,
                       int n) // n will be equal to the value of id
{
  std::set<int> secure;
  std::set<int> notSecure;
  std::vector<int> visited(n, 0);
  for (auto i : ids) {
    std::queue<int> q;
    q.push(i.first);
    while (!q.empty()) {
      int x = q.front();
      q.pop();
      visited[x] = 1;
      secure.insert(x);
      for (auto it : CDG[x]) {
        if (!visited[it]) {
          q.push(it);
        }
      }
    }
  }
  for (int i = 0; i < n; i++) {
    if (visited[i] == 0)
      notSecure.insert(i);
  }

  // printing secure basicblocks
  errs() << "\nSecure: ";
  for (auto it : secure) {
    errs() << it << ", ";
  }

  // printing not secure basicblocks
  errs() << "\nNot Secure: ";
  for (auto it : notSecure) {
    errs() << it << ", ";
  }
}

std::pair<std::set<int>, std::set<int>>
get_secure_BB(std::map<int, std::set<std::pair<int, Value *>>> &DDG,
              std::set<std::pair<int, Value *>> classified, int n) {

  std::set<int> secure, notSecure;
  std::set<std::pair<int, Value *>> visited;
  std::queue<std::pair<int, Value *>> q;
  for (auto c : classified) {
    q.push(c);
  }
  while (!q.empty()) {
    auto it = q.front();
    secure.insert(it.first);
    q.pop();
    if (visited.find(it) == visited.end()) {
      visited.insert(it);
      for (auto jt : DDG.at(it.first)) {
        if (jt.second == it.second) {
          q.push(jt);
        }
      }
    }
  }

  for (int i = 0; i < n; i++) {
    if (secure.find(i) == secure.end()) {
      notSecure.insert(i);
    }
  }

  return std::pair(secure, notSecure);
}

namespace {
struct PDGPass : public ModulePass {
  static char ID;
  PDGPass() : ModulePass(ID) {}

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
        errs() << "\n------------------------------------" << BB
               << "------------------------------------------\n";
      }
    }
    // errs() << "size of bb_rw: " << bb_rw.size() << "\n";
    for (auto &F : M) {
      for (auto &BB : F) {
        // errs() << "\n" << std::stoi(BB.getName().str()) << "->";
        // RAW Dependencies *Start*
        for (BasicBlock *BB_child : successors(&BB)) {
          for (auto r : bb_rw.at(BB_child).first) {
            if (bb_rw.at(&BB).second.find(r) != bb_rw.at(&BB).second.end()) {
              if (&BB != BB_child) {
                // errs() << "\t(" << std::stoi(BB_child->getName().str()) << ",
                // "
                //        << *r << ", RAWDBBn";
                DDG.at(std::stoi(BB.getName().str()))
                    .insert(std::pair(std::stoi(BB_child->getName().str()), r));
              }
            }
          }
        }
        // RAW Dependencies *End*
        for (auto &I : BB) {
          if (Value *val = dyn_cast<Value>(&I)) {
            if (secure_vars.find(val) != secure_vars.end()) {
              classified_BB.insert(
                  std::pair(std::stoi(BB.getName().str()), val));
              secure_BB.insert(std::stoi(BB.getName().str()));
            }
          }
          // DefUse Dependencies *Start*
          auto dep_vec = defUseDep(&I);
          for (auto &dep : dep_vec) {
            if (dep.second)
              if (&BB != dep.first) {

                // errs() << "\t(" << std::stoi(dep.first->getName().str()) <<
                // ", "
                //        << *dep.second << ", DefUseDep\n";
                DDG.at(std::stoi(dep.first->getName().str()))
                    .insert(
                        std::pair(std::stoi(BB.getName().str()), dep.second));
              }
          }
        }
      }
    }

    // DDG Creation *End*
    // Secure / Non-secure Partitioning *Start*
    std::pair<std::set<int>, std::set<int>> sns =
        get_secure_BB(DDG, classified_BB, bb_id);

    errs() << "\nClassified: \n";
    for (auto it : classified_BB) {
      errs() << it.first << ", " << *it.second << "\n";
    }

    errs() << "\n\nData Dependency Graph: \n";
    for (auto const &[key, val] : DDG) {
      errs() << "\n" << key << "->";
      for (auto pairs : val) {
        errs() << "\t(" << pairs.first << ", " << *pairs.second << "),\n";
      }
    }

    errs() << "\n Secure: ";
    for (auto it : sns.first) {
      errs() << it << ", ";
    }
    errs() << "\n Not Secure: ";
    for (auto it : sns.second) {
      errs() << it << ", ";
    }

    // CDG Starts From Here
    int id = 0;
    std::map<uintptr_t, int> Addr2Id;
    std::map<int, std::vector<int>> CDG;
    for (auto &F : M) {
      for (auto &BB : F) {
        uintptr_t Address = reinterpret_cast<uintptr_t>(&BB);
        Addr2Id[Address] = id++;
      }
    }
    id = 0;
    for (auto &F : M) {
      for (auto &BB : F) {
        for (BasicBlock *Pred : successors(&BB)) {
          uintptr_t Address = reinterpret_cast<uintptr_t>(Pred);
          CDG[id].push_back(Addr2Id[Address]);
        }
        id++;
      }
    }
    errs() << "\n\nControl Dependency Graph:\n";
    for (auto it : CDG) {
      errs() << it.first << "->";
      for (auto x : it.second) {
        errs() << x << " ";
      }
      errs() << "\n";
    }
    get_secure_BB_CDG(CDG, classified_BB, bb_id);
    return false;
  }
};
} // namespace
char PDGPass::ID = 0;
static RegisterPass<PDGPass> X("pdg-pass", "PDGPass Pass");
