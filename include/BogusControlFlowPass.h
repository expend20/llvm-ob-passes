#ifndef LLVM_TRANSFORMS_BOGUSCONTROLFLOW_BOGUSCONTROLFLOWPASS_H
#define LLVM_TRANSFORMS_BOGUSCONTROLFLOW_BOGUSCONTROLFLOWPASS_H

#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassPlugin.h"

namespace llvm {

class BogusControlFlowPass : public PassInfoMixin<BogusControlFlowPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
  static bool isRequired() { return false; }
};

} // namespace llvm

// Declare the pass plugin info function
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo();

#endif // LLVM_TRANSFORMS_BOGUSCONTROLFLOW_BOGUSCONTROLFLOWPASS_H