#include <string>
#include <vector>

#include "llvm/ADT/APInt.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h" // For JIT to be linked in
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/Linker/Linker.h>

#include "Driver.hpp"

int main(int argc, char *argv[]) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetAsmPrinter();

  Driver driver{};

  std::vector<std::string> args{};

  bool print_canonical = false;
  bool print_module = false;

  for (size_t i = 1; i < argc; ++i) {
    if (argv[i] == std::string("-p")) {
      driver.trace_parsing = true;
    } else if (argv[i] == std::string("-s")) {
      driver.trace_scanning = true;
    } else if (argv[i] == std::string("-c")) {
      print_canonical = true;
    } else if (argv[i] == std::string("-m")) {
      print_module = true;
    } else {
      args.push_back(argv[i]);
    }
  }

  if (args.empty()) {
    std::cout << "Usage: " << argv[0] << " <source> [arg]" << "\n";
    std::exit(-1);
  }

  std::cout << "Parsing... ";
  driver.parse(args[0]);
  std::cout << "OK" << "\n";

  if (print_canonical) { // AST inspection
    std::cout << "Canonical representation... " << "\n"
              << "---------" << "\n"
              << driver.prg_string() << "\n"
              << "---------" << "\n"
              << "\n";
  }

  std::cout << "Generating LLVM IR... ";
  driver.generate_llvm();
  std::cout << "OK" << "\n";

  if (print_module) {
    std::cout << "The module:" << "\n"
              << "---------" << "\n";
    driver.ctx.module->print(llvm::outs(), nullptr);
    std::cout << "---------" << "\n";
  }

  std::cout << "Verifying... ";
  if (llvm::verifyModule(*driver.ctx.module, &llvm::outs())) {
    llvm::errs() << argv[0] << ": Error constructing function!";
    return 1;
  }
  std::cout << "OK" << "\n";

  std::cout << "Building execution engine... ";
  std::string err_str;
  llvm::ExecutionEngine *execution_engine = llvm::EngineBuilder(std::move(driver.ctx.module))
                                                .setEngineKind(llvm::EngineKind::JIT)
                                                .setErrorStr(&err_str)
                                                .create();

  // std::cout << "Extending global mapping... " << "\n";
  // for (auto &elem : driver.llvm.foundation_fn_map) {
  //   auto fn_ptr = elem.second;
  //   std::cout << "\tAdded: " << fn_ptr->name << "\n";
  //   execution_engine->addGlobalMapping(fn_ptr->name, fn_ptr->global_map_addr());
  // }

  if (!execution_engine) {
    std::cout << "Failed to construct execution engine: " << err_str << "\n";
    return 1;
  } else {
    std::cout << "OK" << "\n";
  }

  auto main_ptr = execution_engine->getFunctionAddress("main");
  if (!main_ptr) {
    throw std::logic_error("Failed to identify main fn for JIT");
  }

  // For simplificty, always pass a default arg, as it'll be ignored if not needed.
  int64_t arg = 0;
  if (1 < args.size()) {
    arg = std::stoll(args[1]);
  }
  if (2 < args.size()) {
    std::cout << "Note: Only zero or one arguments are supported, all others are ignored..." << "\n";
  }

  int64_t (*main)(int64_t) = (int64_t (*)(int64_t))main_ptr;

  std::cout << "Executing..." << "\n"
            << "------" << "\n";
  int64_t exit_code = main(arg);
  std::cout << "\n"
            << "------" << "\n"
            << "Exit code: " << exit_code << "\n";
}
