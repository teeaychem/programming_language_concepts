#include <cstdint>
#include <map>
#include <memory>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "AST/AST.hpp"
#include "AST/Types.hpp"
#include "LLVMBundle.hpp"

extern "C" {
void printi(int64_t i) {
  printf("%lld ", i);
}
}

struct PrintI : FnPrimative {

  PrintI() {
    this->name = "printi";
    this->return_type = std::make_shared<AST::Typ::Void>(AST::Typ::Void());
    this->args = AST::ParamVec{{"n", AST::Typ::pk_Int()}};
  }

  llvm::Function *codegen(LLVMBundle &bundle) const override {
    // TODO: Unify
    auto MC_INT = (llvm::Type *)llvm::Type::getInt64Ty(*bundle.context);

    auto typ = llvm::FunctionType::get(llvm::PointerType::getUnqual(MC_INT), llvm::ArrayRef(MC_INT), false);

    auto fn = llvm::Function::Create(typ, llvm::Function::ExternalLinkage, this->name, bundle.module.get());
    fn->setCallingConv(llvm::CallingConv::C);

    return fn;
  }

  int64_t global_map_addr() const override { return (int64_t)(printi); };
};

extern "C" {

void println() {
  printf("\n");
}
}

struct PrintLn : FnPrimative {

  PrintLn() {
    this->name = "println";
    this->return_type = std::make_shared<AST::Typ::Void>(AST::Typ::Void());
    this->args = AST::ParamVec{};
  }

  llvm::Function *codegen(LLVMBundle &bundle) const override {
    auto typ = llvm::FunctionType::get(llvm::PointerType::getUnqual(llvm::Type::getVoidTy(*bundle.context)), false);

    auto fn = llvm::Function::Create(typ, llvm::Function::ExternalLinkage, "println", bundle.module.get());
    fn->setCallingConv(llvm::CallingConv::C);

    return fn;
  }

  int64_t global_map_addr() const override { return (int64_t)(println); };
};

void extend_ops_foundation(LLVMBundle &bundle) {

  auto printi = std::make_shared<PrintI>(PrintI());
  auto println = std::make_shared<PrintLn>(PrintLn());

  bundle.foundation_fn_map[printi->name] = printi;
  bundle.foundation_fn_map[println->name] = println;
}
