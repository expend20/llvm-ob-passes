// Credits: https://github.com/bluesadi/Pluto/
#include "Pluto/IndirectCall.h"

#include "llvm/IR/IRBuilder.h"
#include <algorithm>
#include <random>
#include <vector>

namespace Pluto {

PreservedAnalyses Pluto::IndirectCall::run(Module &M, ModuleAnalysisManager &AM) {
    LLVMContext &context = M.getContext();
    IRBuilder<> builder(context);

    std::vector<Function *> functions;
    for (Function &F : M) {
        if (F.size() && (F.hasInternalLinkage() || F.hasPrivateLinkage())) {
            functions.push_back(&F);
        }
    }

    std::random_device rd;
    std::default_random_engine rng(rd());
    std::shuffle(functions.begin(), functions.end(), rng);

    std::vector<Constant *> funcAddrs;
    for (Function *F : functions) {
        funcAddrs.push_back(ConstantExpr::getBitCast(F, Type::getInt8Ty(context)->getPointerTo()));
    }

    // Save function addresses to global variable
    ArrayRef<Constant *> funcAddrsRef(funcAddrs);
    ArrayType *functionTableType = ArrayType::get(Type::getInt8Ty(context)->getPointerTo(), funcAddrs.size());
    Constant *funcAddrsArray = ConstantArray::get(functionTableType, funcAddrsRef);
    GlobalVariable *functionTable =
        new GlobalVariable(M, functionTableType, false, GlobalVariable::PrivateLinkage, funcAddrsArray);

    for (Function &F : M) {
        for (BasicBlock &BB : F) {
            for (Instruction &I : BB) {
                if (CallInst *CI = dyn_cast<CallInst>(&I)) {
                    Function *callee = CI->getCalledFunction();
                    auto ptr = std::find(functions.begin(), functions.end(), callee);
                    if (ptr != functions.end()) {
                        builder.SetInsertPoint(&F.getEntryBlock().front());

                        // Find the correct index of current callee
                        int calleeIndex = ptr - functions.begin();

                        // Make index able to be obfuscated by MBAObfuscation
                        AllocaInst *indexPtr = builder.CreateAlloca(Type::getInt32Ty(context));
                        builder.CreateStore(ConstantInt::get(Type::getInt32Ty(context), 0), indexPtr);
                        Value *index = builder.CreateAdd(builder.CreateLoad(indexPtr->getAllocatedType(), indexPtr),
                                                         ConstantInt::get(Type::getInt32Ty(context), calleeIndex));

                        // Replace the original called operand
                        auto GEP = builder.CreateGEP(functionTableType, functionTable,
                                                     {ConstantInt::get(Type::getInt32Ty(context), 0), index});
                        CI->setCalledOperand(builder.CreateLoad(functionTableType->getElementType(), GEP));
                    }
                }
            }
        }
    }
    PreservedAnalyses PA;
    PA.preserveSet<CFGAnalyses>();
    return PA;
}

}; // namespace Pluto