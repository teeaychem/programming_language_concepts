#include "AST/AST.hpp"
#include "LLVMBundle.hpp"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <cstdio>
#include <iostream>
#include <map>

namespace ops_unary {
// Unary subtraction
OpUnary builder_sub(LLVMBundle &bundle) {
  OpUnary builder = [&bundle](AST::ExprHandle expr) {
    llvm::Value *expr_val = expr->codegen(bundle);
    expr_val = bundle.ensure_loaded(expr->type(), expr_val);

    return bundle.builder.CreateMul(llvm::ConstantInt::get(expr_val->getType(), -1), expr_val, "sub");
  };
  return builder;
}

// Logical negation
OpUnary builder_not(LLVMBundle &bundle) {
  OpUnary builder = [&bundle](AST::ExprHandle expr) {
    llvm::Value *expr_val = expr->codegen(bundle);

    expr_val = bundle.ensure_loaded(expr->type(), expr_val);

    return bundle.builder.CreateNot(expr_val);
  };
  return builder;
}

// Print an integer
OpUnary builder_printi(LLVMBundle &bundle) {
  OpUnary builder = [&bundle](AST::ExprHandle expr) {
    std::cout << "printi: " << expr->to_string(0) << "\n";
    llvm::Value *expr_val = expr->codegen(bundle);

    expr_val = bundle.ensure_loaded(expr->type(), expr_val);

    std::vector<llvm::Value *>
        arg_vs{
            bundle.builder.CreateGlobalString("%d\n", "digit_formatter"),
            expr_val,
        };

    return bundle.builder.CreateCall(bundle.foundation_fn_map["printf"], arg_vs);
  };
  return builder;
}

// Print a character
OpUnary builder_printc(LLVMBundle &bundle) {
  OpUnary builder = [&bundle](AST::ExprHandle expr) {
    llvm::Value *expr_val = expr->codegen(bundle);

    std::vector<llvm::Value *> arg_vs{
        bundle.builder.CreateGlobalString("%c", "new_line"),
        expr_val};

    return bundle.builder.CreateCall(bundle.foundation_fn_map["printf"], arg_vs);
  };
  return builder;
}

// Dereference
OpUnary builder_star(LLVMBundle &bundle) {
  OpUnary builder = [&bundle](AST::ExprHandle expr) {
    // FIXME: A copy of access deref for the moment
    llvm::Value *expr_val = expr->codegen(bundle);

    return bundle.builder.CreateLoad(expr->type()->typegen(bundle), expr_val);
  };
  return builder;
}

// Address
OpUnary builder_amp(LLVMBundle &bundle) {
  OpUnary builder = [&bundle](AST::ExprHandle expr) {
    // FIXME: A copy of access deref for the moment
    llvm::Value *expr_val = expr->codegen(bundle);

    return expr_val;
  };
  return builder;
}

} // namespace ops_unary

void extend_ops_unary(LLVMBundle &bundle, OpsUnaryMap &op_map) {

  op_map["-"] = ops_unary::builder_sub(bundle);
  op_map["!"] = ops_unary::builder_not(bundle);

  op_map["*"] = ops_unary::builder_star(bundle);
  op_map["&"] = ops_unary::builder_amp(bundle);

  op_map["printi"] = ops_unary::builder_printi(bundle);
  op_map["printc"] = ops_unary::builder_printc(bundle);
}
