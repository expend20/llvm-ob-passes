#include "ExamplePass.h"
#include "llvm/IR/Function.h"
#include "llvm/Passes/PassBuilder.h"
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