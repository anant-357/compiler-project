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
#include <string>
#include <utility>
#include <vector>

#define NO_DEP 0
#define DEF_USE 1
#define RAW 2

using namespace llvm;

std::vector<std::pair<Instruction *, uint>> defUseDep(Instruction *I) {
  std::vector<std::pair<Instruction *, uint>> res;
  for (Instruction::const_op_iterator cuit = I->op_begin(); cuit != I->op_end();
       ++cuit) {
    errs() << "Operand: " << *(cuit->get()) << " ,\t";
    if (Instruction *inst = dyn_cast<Instruction>(*cuit)) {
      res.push_back(std::pair(inst, cuit->getOperandNo() + 1));
    }
  }
  return res;
}

std::vector<std::pair<Instruction *, uint>> RAWDep(Instruction *I){};

namespace {
struct CustomModulePass : public ModulePass {
  static char ID;
  CustomModulePass() : ModulePass(ID) {}

  bool runOnModule(Module &M) override {
    int bb_id = 0;
    std::map<std::string, std::set<std::string>> DDG;
    std::map<std::string, std::pair<std::set<StringRef>, std::set<StringRef>>>
        bb_rw;
    int n_bb = 0;
    for (auto &F : M) {
      for (auto &BB : F) {
        BB.setName(std::to_string(n_bb++));
      }
    }
    for (auto &F : M) {
      for (auto &BB : F) {
        std::pair<std::set<StringRef>, std::set<StringRef>> read_write;
        errs() << "\nBasic Block ID: " << bb_id
               << "\n-------------------------------------\n"
               << BB << "\n-------------------------------------\n";
        for (auto &I : BB) {
          // auto dep_vec = defUseDep(&I);
          //  for (auto &dep : dep_vec) {
          //    if (dep.second != 0)
          //      errs() << "\tfound: definition->" << *dep.first
          //             << ", operand no. -> " << dep.second - 1 << "\t\t";
          //  }
          // errs() << "instruction:" << I << "\n";
        }
        // errs() << "Read Variables:\n";
        // for (auto val : read_write.first) {
        //   errs() << "- " << val << "\n";
        // }
        // errs() << "Written Variables:\n";
        // for (auto val : read_write.second) {
        //   errs() << "- " << val << "\n";
        // }
        bb_rw[BB.getName().str()] = read_write;
        bb_id++;
      }
    }
    return false;
  }
};
} // namespace
char CustomModulePass::ID = 0;
static RegisterPass<CustomModulePass> X("custom-module-pass",
                                        "CustomModulePass Pass");
