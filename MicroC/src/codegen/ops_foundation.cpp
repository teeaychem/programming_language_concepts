#include <map>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "LLVMBundle.hpp"

namespace ops_foundation {
OpFoundation builder_printi(LLVMBundle &bundle) {

  auto MC_INT = (llvm::Type *)llvm::Type::getInt64Ty(*bundle.context);

  auto typ = llvm::FunctionType::get(llvm::PointerType::getUnqual(MC_INT), llvm::ArrayRef(MC_INT), false);

  auto fn = llvm::Function::Create(typ, llvm::Function::ExternalLinkage, "printi", bundle.module.get());
  fn->setCallingConv(llvm::CallingConv::C);

  return fn;
}

OpFoundation builder_println(LLVMBundle &bundle) {

  auto typ = llvm::FunctionType::get(llvm::PointerType::getUnqual(llvm::Type::getVoidTy(*bundle.context)), false);

  auto fn = llvm::Function::Create(typ, llvm::Function::ExternalLinkage, "println", bundle.module.get());
  fn->setCallingConv(llvm::CallingConv::C);

  return fn;
}

} // namespace ops_foundation
void extend_ops_foundation(LLVMBundle &bundle, OpsFoundationMap &op_map) {

  op_map["printi"] = ops_foundation::builder_printi(bundle);
  op_map["println"] = ops_foundation::builder_println(bundle);
}
