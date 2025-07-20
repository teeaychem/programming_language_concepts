#include "LLVMBundle.hpp"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <map>

namespace ops_unary {
OpUnary builder_sub(LLVMBundle &bundle) {
  OpUnary builder = [&bundle](llvm::Value *expr) {
    return bundle.builder.CreateMul(llvm::ConstantInt::get(expr->getType(), -1), expr, "sub");
  };
  return builder;
}

OpUnary builder_printi(LLVMBundle &bundle) {
  OpUnary builder = [&bundle](llvm::Value *expr) {
    std::vector<llvm::Value *> arg_vs{
        bundle.builder.CreateGlobalString("%d\n", "digit_formatter"),
        expr,
    };

    return bundle.builder.CreateCall(bundle.foundation_fn_map["printf"], arg_vs);
  };
  return builder;
}

OpUnary builder_printc(LLVMBundle &bundle) {
  OpUnary builder = [&bundle](llvm::Value *expr) {
    std::vector<llvm::Value *> arg_vs{
        bundle.builder.CreateGlobalString("%c", "new_line"),
        expr};

    return bundle.builder.CreateCall(bundle.foundation_fn_map["printf"], arg_vs);
  };
  return builder;
}

} // namespace ops_unary

// namespace codegen

void extend_ops_unary(LLVMBundle &bundle, OpsUnaryMap &op_map) {

  op_map["-"] = ops_unary::builder_sub(bundle);
  op_map["printi"] = ops_unary::builder_printi(bundle);
  op_map["printc"] = ops_unary::builder_printc(bundle);
}
