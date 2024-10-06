// Credits: https://github.com/bluesadi/Pluto/
#pragma once

#include "llvm/IR/PassManager.h"

using namespace llvm;

namespace Pluto {

struct IndirectCall : PassInfoMixin<IndirectCall> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);

    static bool isRequired() { return true; }
};

}; // namespace Pluto