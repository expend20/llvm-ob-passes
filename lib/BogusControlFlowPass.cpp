// Credits: https://github.com/bluesadi/Pluto/
#include "BogusControlFlowPass.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include <vector>
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

namespace {

BasicBlock *cloneBasicBlock(BasicBlock *BB) {
    ValueToValueMapTy VMap;
    BasicBlock *cloneBB = CloneBasicBlock(BB, VMap, "cloneBB", BB->getParent());
    BasicBlock::iterator origI = BB->begin();
    // Fix references in the cloned basic block
    for (Instruction &I : *cloneBB) {
        for (int i = 0; i < I.getNumOperands(); i++) {
            Value *V = MapValue(I.getOperand(i), VMap);
            if (V) {
                I.setOperand(i, V);
            }
        }
        SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
        I.getAllMetadata(MDs);
        for (std::pair<unsigned, MDNode *> pair : MDs) {
            MDNode *MD = MapMetadata(pair.second, VMap);
            if (MD) {
                I.setMetadata(pair.first, MD);
            }
        }
        I.setDebugLoc(origI->getDebugLoc());
        origI++;
    }
    return cloneBB;
}

Value *createBogusCmp(BasicBlock *insertAfter) {
    // if((y < 10 || x * (x + 1) % 2 == 0))
    Module *M = insertAfter->getModule();
    LLVMContext &context = M->getContext();
    GlobalVariable *xptr = new GlobalVariable(*M, Type::getInt32Ty(context), false, GlobalValue::CommonLinkage,
                                              ConstantInt::get(Type::getInt32Ty(context), 0), "x");
    GlobalVariable *yptr = new GlobalVariable(*M, Type::getInt32Ty(context), false, GlobalValue::CommonLinkage,
                                              ConstantInt::get(Type::getInt32Ty(context), 0), "y");

    IRBuilder<> builder(context);
    builder.SetInsertPoint(insertAfter);
    LoadInst *x = builder.CreateLoad(Type::getInt32Ty(context), xptr);
    LoadInst *y = builder.CreateLoad(Type::getInt32Ty(context), yptr);
    Value *cond1 = builder.CreateICmpSLT(y, ConstantInt::get(Type::getInt32Ty(context), 10));
    Value *op1 = builder.CreateAdd(x, ConstantInt::get(Type::getInt32Ty(context), 1));
    Value *op2 = builder.CreateMul(op1, x);
    Value *op3 = builder.CreateURem(op2, ConstantInt::get(Type::getInt32Ty(context), 2));
    Value *cond2 = builder.CreateICmpEQ(op3, ConstantInt::get(Type::getInt32Ty(context), 0));
    return BinaryOperator::CreateOr(cond1, cond2, "", insertAfter);
}

} // anonymous namespace

PreservedAnalyses BogusControlFlowPass::run(Function &F, FunctionAnalysisManager &AM) {
    std::vector<BasicBlock *> origBB;
    for (BasicBlock &BB : F) {
        origBB.push_back(&BB);
    }
    for (BasicBlock *BB : origBB) {
        if (isa<InvokeInst>(BB->getTerminator()) || BB->isEHPad()) {
            continue;
        }
        // Step 1: Split into headBB, bodyBB, endBB
        BasicBlock *headBB = BB;
        BasicBlock *bodyBB = BB->splitBasicBlock(BB->getFirstNonPHIOrDbgOrLifetime(), "bodyBB");
        BasicBlock *tailBB = bodyBB->splitBasicBlock(bodyBB->getTerminator(), "endBB");
        
        // Step 2: Clone bodyBB to get cloneBB
        BasicBlock *cloneBB = cloneBasicBlock(bodyBB);

        // Step 3: Construct bogus jumps
        // 3.1: Remove absolute jumps from entryBB, bodyBB, cloneBB
        BB->getTerminator()->eraseFromParent();
        bodyBB->getTerminator()->eraseFromParent();
        cloneBB->getTerminator()->eraseFromParent();
        
        // 3.2: Insert bogus comparison instructions at the end of entryBB and bodyBB
        Value *cond1 = createBogusCmp(BB);
        Value *cond2 = createBogusCmp(bodyBB);
        
        // 3.3: Change absolute jump from entryBB to bodyBB to conditional jump
        BranchInst::Create(bodyBB, cloneBB, cond1, BB);
        
        // 3.4: Change absolute jump from bodyBB to endBB to conditional jump
        BranchInst::Create(tailBB, cloneBB, cond2, bodyBB);
        
        // 3.5: Add absolute jump from bodyBB.clone to bodyBB
        BranchInst::Create(bodyBB, cloneBB);
    }
    return PreservedAnalyses::none();
}

// At the end of the file
PassPluginLibraryInfo getBogusControlFlowPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "BogusControlFlowPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "bogus-control-flow") {
                        FPM.addPass(BogusControlFlowPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getBogusControlFlowPassPluginInfo();
}