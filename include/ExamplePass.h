#ifndef EXAMPLE_PASS_H
#define EXAMPLE_PASS_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class ExamplePass : public PassInfoMixin<ExamplePass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
  static bool isRequired() { return true; }
};

} // namespace llvm

#endif // EXAMPLE_PASS_H
