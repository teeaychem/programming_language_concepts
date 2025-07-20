#include "LLVMBundle.hpp"

#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <map>

namespace ops_foundation {
OpFoundation builder_printf(LLVMBundle &bundle) {

  return llvm::Function::Create(llvm::FunctionType::get(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*bundle.context)), true), llvm::Function::ExternalLinkage, "printf", bundle.module.get());
}

} // namespace ops_foundation
void extend_ops_foundation(LLVMBundle &bundle, OpsFoundationMap &op_map) {

  op_map["printf"] = ops_foundation::builder_printf(bundle);
}
