// Credits: https://github.com/bluesadi/Pluto/
#ifndef LLVM_TRANSFORMS_BOGUSCONTROLFLOW_BOGUSCONTROLFLOWPASS_H
#define LLVM_TRANSFORMS_BOGUSCONTROLFLOW_BOGUSCONTROLFLOWPASS_H

#include "llvm/IR/PassManager.h"

using namespace llvm;

namespace Pluto {

class BogusControlFlowPass : public PassInfoMixin<BogusControlFlowPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
  static bool isRequired() { return false; }
};

} // namespace Pluto

#endif // LLVM_TRANSFORMS_BOGUSCONTROLFLOW_BOGUSCONTROLFLOWPASS_H