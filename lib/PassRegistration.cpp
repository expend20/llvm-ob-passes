#include "Pluto/BogusControlFlowPass.h"
#include "Pluto/Flattening.h"
#include "Pluto/GlobalEncryption.h"
#include "ExamplePass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

// Combined registration function
static void registerPasses(PassBuilder &PB) {
  // Register the module pass (GlobalEncryption)
  PB.registerPipelineParsingCallback(
    [](StringRef Name, ModulePassManager &MPM,
       ArrayRef<PassBuilder::PipelineElement>) {
      if (Name == "pluto-global-encryption") {
        MPM.addPass(Pluto::GlobalEncryption());
        return true;
      }
      return false;
    });

  // Register the function passes
  PB.registerPipelineParsingCallback(
    [](StringRef Name, FunctionPassManager &FPM,
       ArrayRef<PassBuilder::PipelineElement>) {
      if (Name == "example-pass") {
        FPM.addPass(ExamplePass());
        return true;
      }
      if (Name == "pluto-bogus-control-flow") {
        FPM.addPass(Pluto::BogusControlFlowPass());
        return true;
      }
      if (Name == "pluto-flattening") {
        FPM.addPass(Pluto::Flattening());
        return true;
      }
      return false;
    });
}

// This is the core interface for pass plugins.
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "CombinedPasses", LLVM_VERSION_STRING,
    [](PassBuilder &PB) { registerPasses(PB); }
  };
}