// #include "mCLLVM.hh"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <stdlib.h>
#include <string>

#include "Driver.hpp"

#include "llvm/ADT/APInt.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h" // For JIT to be linked in
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Linker/Linker.h>

#include <llvm/IR/GlobalVariable.h>

extern "C" {
void printi(int64_t i) {
  printf("%lld ", i);
}
}

int main(int argc, char *argv[]) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetAsmPrinter();

  std::printf("Scratch, for the moment\n");

  Driver driver;

  for (size_t i = 1; i < argc; ++i) {
    if (argv[i] == std::string("-p")) {
      driver.trace_parsing = true;
    } else if (argv[i] == std::string("-s")) {
      driver.trace_scanning = true;
    } else {
      std::cout << "Parsing... " << "\n";
      driver.parse(argv[i]);
      std::cout << "Parsing OK" << "\n";
    }
  }

  { // AST inspection
    std::cout << "Printing string representation... ";
    std::fflush(stdout);
    printf("%s\n", driver.prg_string().c_str());
    std::cout << "OK" << "\n";
    std::fflush(stdout);
  }

  std::cout << "Generating LLVM IR... ";
  driver.generate_llvm();
  std::cout << "OK" << "\n";

  std::cout << "The module:" << "\n"
            << "\n"
            << "---------" << "\n";
  driver.llvm.module->print(llvm::outs(), nullptr);
  std::cout << "---------" << "\n"
            << "\n";

  std::cout << "Verifying... ";

  if (llvm::verifyModule(*driver.llvm.module, &llvm::outs())) {
    llvm::errs() << argv[0] << ": Error constructing function!";
    return 1;
  }

  std::cout << "OK" << "\n";

  std::cout << "Building execution engine... ";
  std::string err_str;
  llvm::ExecutionEngine *execution_engine = llvm::EngineBuilder(std::move(driver.llvm.module))
                                                // .setEngineKind(llvm::EngineKind::JIT)
                                                .setErrorStr(&err_str)
                                                .create();

  execution_engine->addGlobalMapping("printi", (int64_t)(printi));

  if (!execution_engine) {
    std::cout << "Failed to construct execution engine: " << err_str << "\n";
    return 1;
  } else {
    std::cout << "OK" << "\n";
  }

  // auto main = execution_engine->FindFunctionNamed(llvm::StringRef("main"));
  auto main_ptr = execution_engine->getFunctionAddress("main");
  if (!main_ptr) {
    throw std::logic_error("Failed to identify main function for JIT");
  }

  int64_t (*fn)(int64_t) = (int64_t (*)(int64_t))main_ptr;

  std::cout << "Executing..." << "\n";

  std::cout << "------" << "\n"
            << "\n";
  // TODO: Variable argument length

  int64_t GV = fn(0);
  std::cout << "\n"
            << "------" << "\n";
  std::cout << GV << "\n";

  // return result.IntVal.getLimitedValue();
}
