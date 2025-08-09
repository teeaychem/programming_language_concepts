#include <map>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "AST/AST.hpp"
#include "AST/Types.hpp"
#include "LLVMBundle.hpp"

// Contents:
// - Foundation fns in lexicographic order, as extern / FnPrimative pairings.
// - Specification of the foundation fn map.

// Foundation fns

// printi

extern "C" {
void printi(int64_t i) {
  printf("%lld ", i);
}
}

// Prints an integer.
// Equivalent to the `print` statement in microC of PLC, with each `print` parsed to a `printi` call.
struct PrintI : FnPrimative {

  PrintI() {
    this->name = "printi";
    this->return_type = std::make_shared<AST::Typ::Void>(AST::Typ::Void());
    this->args = AST::ArgVec{{"n", AST::Typ::pk_Int()}};
  }

  llvm::Function *codegen(LLVMBundle &bundle) const override {
    auto typ = llvm::FunctionType::get(llvm::Type::getVoidTy(*bundle.context), llvm::ArrayRef(bundle.get_typ(AST::Typ::Kind::Int)), false);

    auto fn = llvm::Function::Create(typ, llvm::Function::ExternalLinkage, this->name, bundle.module.get());
    fn->setCallingConv(llvm::CallingConv::C);

    return fn;
  }

  int64_t global_map_addr() const override { return (int64_t)(printi); };
};

// println
extern "C" {

void println() {
  printf("\n");
}
}

// Prints a new line.
// Equivalent to the `println` statement in microC of PLC, with each `println` parsed to a `println` call.
struct PrintLn : FnPrimative {

  PrintLn() {
    this->name = "println";
    this->return_type = std::make_shared<AST::Typ::Void>(AST::Typ::Void());
    this->args = AST::ArgVec{};
  }

  llvm::Function *codegen(LLVMBundle &bundle) const override {
    auto typ = llvm::FunctionType::get(llvm::Type::getVoidTy(*bundle.context), false);

    auto fn = llvm::Function::Create(typ, llvm::Function::ExternalLinkage, "println", bundle.module.get());
    fn->setCallingConv(llvm::CallingConv::C);

    return fn;
  }

  int64_t global_map_addr() const override { return (int64_t)(println); };
};

// Specification of the foundation fn map
void LLVMBundle::populate_foundation_fn_map() {

  auto printi = std::make_shared<PrintI>(PrintI());
  auto println = std::make_shared<PrintLn>(PrintLn());

  this->foundation_fn_map[printi->name] = printi;
  this->foundation_fn_map[println->name] = println;
}
