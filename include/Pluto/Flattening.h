// Credits: https://github.com/bluesadi/Pluto
#pragma once

#include "llvm/IR/PassManager.h"

using namespace llvm;

namespace Pluto {

struct Flattening : PassInfoMixin<Flattening> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

    static bool isRequired() { return true; }
};

}; // namespace Pluto