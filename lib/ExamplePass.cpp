#include "ExamplePass.h"
#include "llvm/IR/Function.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

// Command line option
static cl::opt<unsigned> BBThreshold("bb-threshold",
  cl::desc("Threshold for the number of basic blocks"),
  cl::init(5));

PreservedAnalyses ExamplePass::run(Function &F, FunctionAnalysisManager &AM) {
  errs() << "ExamplePass: Running on function " << F.getName() << "\n";
  
  // Your pass implementation goes here
  // For this example, we'll just count the number of basic blocks
  int bbCount = 0;
  for (auto &BB : F) {
    bbCount++;
  }
  
  errs() << "ExamplePass: Function " << F.getName() << " has " << bbCount << " basic blocks.\n";

  // Check if the number of basic blocks exceeds the threshold
  if (bbCount > BBThreshold) {
    errs() << "ExamplePass: Warning - Function " << F.getName() 
           << " exceeds the basic block threshold of " << BBThreshold << "\n";
  }

  return PreservedAnalyses::all();
}

// Pass registration
static void registerExamplePass(PassBuilder &PB) {
  PB.registerPipelineParsingCallback(
    [](StringRef Name, FunctionPassManager &FPM,
       ArrayRef<PassBuilder::PipelineElement>) {
      if (Name == "example-pass") {
        FPM.addPass(ExamplePass());
        return true;
      }
      return false;
    });
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize ExamplePass when added to the pass pipeline on the
// command line, i.e. via '-passes=example-pass'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "ExamplePass", LLVM_VERSION_STRING,
    [](PassBuilder &PB) { registerExamplePass(PB); }
  };
}