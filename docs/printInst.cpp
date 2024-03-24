// Commands
// clang++ -shared -fPIC printInst.cpp -o printInst.so `llvm-config --cxxflags --ldflags --libs`
// opt -load ./printInst.so -printInst < filename.ll

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <string>
#include <vector>
#include <list>


using namespace llvm;

namespace {

    struct PrintInst : public ModulePass {
    static char ID;

    PrintInst() : ModulePass(ID) {}

    bool runOnModule(Module &M) override {
        for (auto &F : M) { 
            for (auto &BB : F) {
                for (auto &Inst : BB) {
			errs() << "Inst => " << Inst << "\n";
			//Inst.getOpcode();
                        //Inst.getOprand();
                         
                }
            }
        }
        return false;
    }
    };
}

char PrintInst::ID = 0;
static RegisterPass<PrintInst> X("printInst", "Display the instruction.");
