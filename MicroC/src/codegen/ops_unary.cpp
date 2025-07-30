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
FnUnary builder_sub(LLVMBundle &bundle) {
  FnUnary builder = [&bundle](AST::ExprHandle expr) {
    llvm::Value *expr_val = expr->codegen(bundle);
    expr_val = bundle.ensure_loaded(expr->type(), expr_val);

    return bundle.builder.CreateMul(llvm::ConstantInt::get(expr_val->getType(), -1), expr_val, "sub");
  };
  return builder;
}

// Logical negation
FnUnary builder_not(LLVMBundle &bundle) {
  FnUnary builder = [&bundle](AST::ExprHandle expr) {
    llvm::Value *expr_val = expr->codegen(bundle);

    expr_val = bundle.ensure_loaded(expr->type(), expr_val);

    return bundle.builder.CreateNot(expr_val);
  };
  return builder;
}

// Print an integer
FnUnary builder_printi(LLVMBundle &bundle) {
  FnUnary builder = [&bundle](AST::ExprHandle expr) {
    llvm::Value *expr_val = expr->codegen(bundle);

    expr_val = bundle.ensure_loaded(expr->type(), expr_val);

    return bundle.builder.CreateCall(bundle.foundation_fn_map["printi"], expr_val);
  };
  return builder;
}

// Dereference
FnUnary builder_star(LLVMBundle &bundle) {
  FnUnary builder = [&bundle](AST::ExprHandle expr) {
    // FIXME: A copy of access deref for the moment
    llvm::Value *expr_val = expr->codegen(bundle);

    return bundle.builder.CreateLoad(expr->type()->typegen(bundle), expr_val);
  };
  return builder;
}

// Address
FnUnary builder_amp(LLVMBundle &bundle) {
  FnUnary builder = [&bundle](AST::ExprHandle expr) {
    // FIXME: A copy of access deref for the moment
    llvm::Value *expr_val = expr->codegen(bundle);

    return expr_val;
  };
  return builder;
}

} // namespace ops_unary

void extend_ops_unary(LLVMBundle &bundle, OpsUnaryMap &op_map) {

  op_map[AST::Expr::OpUnary::Minus] = ops_unary::builder_sub(bundle);
  op_map[AST::Expr::OpUnary::Negation] = ops_unary::builder_not(bundle);

  op_map[AST::Expr::OpUnary::Dereference] = ops_unary::builder_star(bundle);
  op_map[AST::Expr::OpUnary::AddressOf] = ops_unary::builder_amp(bundle);
}
