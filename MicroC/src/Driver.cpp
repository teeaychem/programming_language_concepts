#include <cstdio>
#include <memory>
#include <sstream>

#include "Driver.hpp"
#include "codegen/LLVMBundle.hpp"
#include "parser.hpp"

int Driver::parse(const std::string &f) {
  file = f;
  location.initialize(&file);
  int res;
  {
    scan_begin();
    yy::parser parse(*this);
    parse.set_debug_level(trace_parsing);
    res = parse();
    scan_end();
  }
  return res;
}

std::string Driver::prg_string() {
  std::stringstream prg_ss{};
  prg_ss << "\n\n";
  for (auto &dec : prg) {
    prg_ss << dec->to_string(0);
    prg_ss << "\n\n";
  }

  return prg_ss.str();
}

void Driver::generate_llvm() {
  for (auto &dec : prg) {
    dec->codegen(llvm);
  }
};

void Driver::print_llvm() {
  printf("\n----------\n");
  llvm.module->print(llvm::outs(), nullptr);
  printf("\n----------\n");
}
 AST::TypHandle Driver::pk_Ptr(TypHandle of) {
    TypPointer type_pointer(std::move(of));
    return std::make_shared<TypPointer>(std::move(type_pointer));
  }
