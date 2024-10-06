#include "ExamplePass.h"

#include "Pluto/BogusControlFlowPass.h"
#include "Pluto/Flattening.h"
#include "Pluto/GlobalEncryption.h"
#include "Pluto/IndirectCall.h"
#include "Pluto/MBAObfuscation.h"
#include "Pluto/Substitution.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/LowerSwitch.h"

using namespace llvm;

struct LowerSwitchWrapper : LowerSwitchPass {
    static bool isRequired() { return true; }
};

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
      if (Name == "pluto-indirect-call") {
        MPM.addPass(Pluto::IndirectCall());
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
        FPM.addPass(LowerSwitchWrapper());
        FPM.addPass(Pluto::Flattening());
        return true;
      }
      if (Name == "pluto-mba-obfuscation") {
        FPM.addPass(Pluto::MbaObfuscation());
        return true;
      }
      if (Name == "pluto-substitution") {
        FPM.addPass(Pluto::Substitution());
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