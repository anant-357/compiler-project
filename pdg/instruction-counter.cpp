#include <bits/stdc++.h>
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


//Function to print the secure and not Secure Blocks
void func(std::map<int,std::vector<int>> &PDG,std::map<int,uintptr_t > &mp1,vector<int> &ids,int n)//n will be equal to the value of id
{
    vector<int> secure;
    vector<int> notSecure;
    vector<int> visited(n,0);
    for(int i=0;i<ids.size();i++)
    {
        queue<int> q;
        q.push(PDG[ids[i]]);
        while(!q.empty())
        {
            int x=q.front();
            q.pop();
            visited[x]=1;
            secure.push_back(x);
            for(auto it:x)
            {
                if(!visited[it]){
                   q.push(it);                   
                }
            }
        }        
    }
    for(int i=0;i<n;i++)
    {
        if(visited[i]==0) notSecure.push_back(i);
    }

    //Printing Secure BasicBlocks
    for(auto it:secure){
        errs()<<mp1[it]<<"\n";
    }

    //Printing Not Secure BasicBlocks
    for(auto it:notSecure){
        errs()<<mp1[it]<<"\n";
    }
}


using namespace llvm;
namespace {
struct InstructionCounter : public ModulePass {
  static char ID;
  InstructionCounter() : ModulePass(ID) {}
  bool runOnModule(Module &M) override {
    int id=0;
    std::map<uintptr_t,int > mp;
    std::map<int,uintptr_t > mp1;
    std::map<int,std::vector<int>> PDG;
    for (auto &F : M) {
        for (auto& BB : F) {
            uintptr_t Address = reinterpret_cast<uintptr_t>(&BB);
            mp1[id]=Address;
            mp[Address]=id++;
        }      
    }
    id=0;
    for (auto &F : M) {
        for (auto& BB : F) {
            for (BasicBlock *Pred : successors(&BB)) {                    
                uintptr_t Address = reinterpret_cast<uintptr_t>(Pred);
                PDG[id].push_back(mp[Address]);               
            }
            id++;
        }      
    }
    errs()<<"Control Dependency Graph:\n";
    for(auto it:PDG)
    {
        errs()<<it.first<<"->";
        for(auto x:it.second){
            errs()<<x<<" ";
        }
        errs()<<"\n";
    }
    return false;
  }
};
} // namespace
char InstructionCounter::ID = 0;
static RegisterPass<InstructionCounter> X("instruction-counter","Instruction Counter Pass");
