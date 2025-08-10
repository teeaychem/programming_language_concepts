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

// The main thing, bundling most tasks.
struct Thing {
  // The source file.
  std::string source;

  // The argument to be passed to main.
  int64_t arg = 0;

  // The JIT engine, built after generating IR with `build_execution_engine`.
  llvm::ExecutionEngine *execution_engine{nullptr};

  // Struct for parsing, and codegen by extension
  Driver driver{};

  // Initialisation from main
  Thing(std::string source, int64_t arg) : source(source), arg(arg) {
  }

  // Print the module to stdout.
  void print_module() {
    std::cout << "The module:" << "\n"
              << "---------" << "\n";
    this->driver.ctx.module->print(llvm::outs(), nullptr);
    std::cout << "---------" << "\n";
  }

  // Print a 'canonical' representation of the source to stdout.
  // This isn't necessarily parsable souce, but is useful for indirect AST inspection.
  void print_canonical() {
    std::cout << "Canonical representation... " << "\n"
              << "---------" << "\n";
    std::cout << this->driver.prg_string() << "\n";
    std::cout << "---------" << "\n"
              << "\n";
  }

  // Parse the source to an AST, held in `driver`.
  void parse() {
    std::cout << "Parsing... ";
    this->driver.parse(this->source);
    std::cout << "OK" << "\n";
  }

  // Generate LLVM IR for an AST.
  void generate_ir() {
    std::cout << "Generating LLVM IR... ";
    this->driver.generate_ir();
    std::cout << "OK" << "\n";
  }

  void verify() {
    std::cout << "Verifying... ";
    if (llvm::verifyModule(*this->driver.ctx.module, &llvm::outs())) {
      llvm::errs() << "Error constructing function!";
      std::exit(1);
    }
    std::cout << "OK" << "\n";
  }

  void build_execution_engine() {
    std::cout << "Building execution engine... ";
    std::string err_str;
    this->execution_engine = llvm::EngineBuilder(std::move(this->driver.ctx.module))
                                 .setEngineKind(llvm::EngineKind::JIT)
                                 .setErrorStr(&err_str)
                                 .create();

    // std::cout << "Extending global mapping... " << "\n";
    // for (auto &elem : bundle.driver.llvm.foundation_fn_map) {
    //   auto fn_ptr = elem.second;
    //   std::cout << "\tAdded: " << fn_ptr->name << "\n";
    //   execution_engine->addGlobalMapping(fn_ptr->name, fn_ptr->global_map_addr());
    // }

    if (!execution_engine) {
      std::cout << "Failed to construct execution engine: " << err_str << "\n";
      std::exit(1);
    } else {
      std::cout << "OK" << "\n";
    }
  }

  //
  void execute_main() {
    if (!this->execution_engine) {
      throw std::logic_error("Execution requires engine");
    }

    auto main_ptr = this->execution_engine->getFunctionAddress("main");
    if (!main_ptr) {
      throw std::logic_error("Failed to identify main fn for JIT");
    }

    int64_t (*main)(int64_t) = (int64_t (*)(int64_t))main_ptr;

    std::cout << "Executing..." << "\n"
              << "------" << "\n";
    int64_t exit_code = main(this->arg);
    std::cout << "\n"
              << "------" << "\n"
              << "Exit code: " << exit_code << "\n";
  }
};

int main(int argc, char *argv[]) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetAsmPrinter();

  bool print_canonical = false;
  bool print_module = false;
  bool trace_parsing = false;
  bool trace_scanning = false;

  std::vector<std::string> args{};

  for (size_t i = 1; i < argc; ++i) {
    if (argv[i] == std::string("-p")) {
      trace_parsing = true;
    } else if (argv[i] == std::string("-s")) {
      trace_scanning = true;
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

  std::string source = args[0];

  // For simplificty, always pass a default arg, as it'll be ignored if not needed.
  int64_t arg = 0;
  if (1 < args.size()) {
    arg = std::stoll(args[1]);
  }

  if (2 < args.size()) {
    std::cout << "Note: Only zero or one arguments are supported, all others are ignored..." << "\n";
  }

  Thing thing(source, arg);

  thing.parse();

  if (print_canonical) {
    thing.print_canonical();
  }

  thing.generate_ir();

  if (print_module) {
    thing.print_module();
  }

  thing.verify();

  thing.build_execution_engine();

  thing.execute_main();
}
