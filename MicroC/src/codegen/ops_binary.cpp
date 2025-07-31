#include <map>

#include "LLVMBundle.hpp"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

#include "AST/AST.hpp"

namespace ops_binary {

FnBinary builder_assign(LLVMBundle &bundle) {
  FnBinary builder = [&bundle](AST::ExprHandle destination, AST::ExprHandle value) {
    llvm::Value *destination_val = destination->codegen(bundle);
    llvm::Value *value_val = value->codegen(bundle);
    value_val = bundle.ensure_loaded(value->type(), value_val);

    return bundle.builder.CreateStore(value_val, destination_val, "op.assign");
  };
  return builder;
}

FnBinary builder_add(LLVMBundle &bundle) {
  FnBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    a_val = bundle.ensure_loaded(a->type(), a_val);
    b_val = bundle.ensure_loaded(b->type(), b_val);

    return bundle.builder.CreateAdd(a_val, b_val, "op.add");
  };
  return builder;
}

FnBinary builder_sub(LLVMBundle &bundle) {
  FnBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    a_val = bundle.ensure_loaded(a->type(), a_val);
    b_val = bundle.ensure_loaded(b->type(), b_val);

    return bundle.builder.CreateSub(a_val, b_val, "op.sub");
  };
  return builder;
}

FnBinary builder_mul(LLVMBundle &bundle) {
  FnBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    a_val = bundle.ensure_loaded(a->type(), a_val);
    b_val = bundle.ensure_loaded(b->type(), b_val);

    return bundle.builder.CreateMul(a_val, b_val, "op.mul");
  };
  return builder;
}

FnBinary builder_div(LLVMBundle &bundle) {
  FnBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    a_val = bundle.ensure_loaded(a->type(), a_val);
    b_val = bundle.ensure_loaded(b->type(), b_val);

    return bundle.builder.CreateSDiv(a_val, b_val, "op.div");
  };
  return builder;
}

FnBinary builder_mod(LLVMBundle &bundle) {
  FnBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    a_val = bundle.ensure_loaded(a->type(), a_val);
    b_val = bundle.ensure_loaded(b->type(), b_val);

    return bundle.builder.CreateSRem(a_val, b_val, "op.mod");
  };
  return builder;
}

FnBinary builder_eq(LLVMBundle &bundle) {
  FnBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    a_val = bundle.ensure_loaded(a->type(), a_val);
    b_val = bundle.ensure_loaded(b->type(), b_val);

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_EQ, a_val, b_val);
  };
  return builder;
}

FnBinary builder_ne(LLVMBundle &bundle) {
  FnBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    a_val = bundle.ensure_loaded(a->type(), a_val);
    b_val = bundle.ensure_loaded(b->type(), b_val);

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_NE, a_val, b_val);
  };
  return builder;
}

FnBinary builder_gt(LLVMBundle &bundle) {
  FnBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    a_val = bundle.ensure_loaded(a->type(), a_val);
    b_val = bundle.ensure_loaded(b->type(), b_val);

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SGT, a_val, b_val);
  };
  return builder;
}

FnBinary builder_lt(LLVMBundle &bundle) {
  FnBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    a_val = bundle.ensure_loaded(a->type(), a_val);
    b_val = bundle.ensure_loaded(b->type(), b_val);

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SLT, a_val, b_val);
  };
  return builder;
}

FnBinary builder_ge(LLVMBundle &bundle) {
  FnBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    a_val = bundle.ensure_loaded(a->type(), a_val);
    b_val = bundle.ensure_loaded(b->type(), b_val);

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SGE, a_val, b_val);
  };
  return builder;
}

FnBinary builder_le(LLVMBundle &bundle) {
  FnBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    a_val = bundle.ensure_loaded(a->type(), a_val);
    b_val = bundle.ensure_loaded(b->type(), b_val);

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SLE, a_val, b_val);
  };
  return builder;
}

FnBinary builder_and(LLVMBundle &bundle) {
  FnBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    a_val = bundle.ensure_loaded(a->type(), a_val);
    b_val = bundle.ensure_loaded(b->type(), b_val);

    return bundle.builder.CreateAnd(a_val, b_val);
  };
  return builder;
}

FnBinary builder_or(LLVMBundle &bundle) {
  FnBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    a_val = bundle.ensure_loaded(a->type(), a_val);
    b_val = bundle.ensure_loaded(b->type(), b_val);

    return bundle.builder.CreateOr(a_val, b_val);
  };
  return builder;
}

} // namespace ops_binary
void extend_ops_binary(LLVMBundle &bundle, OpsBinaryMap &op_map) {
  op_map[AST::Expr::OpBinary::Assign] = ops_binary::builder_assign(bundle);

  op_map[AST::Expr::OpBinary::Add] = ops_binary::builder_add(bundle);
  op_map[AST::Expr::OpBinary::Sub] = ops_binary::builder_sub(bundle);
  op_map[AST::Expr::OpBinary::Mul] = ops_binary::builder_mul(bundle);
  op_map[AST::Expr::OpBinary::Div] = ops_binary::builder_div(bundle);
  op_map[AST::Expr::OpBinary::Mod] = ops_binary::builder_mod(bundle);

  op_map[AST::Expr::OpBinary::Eq] = ops_binary::builder_eq(bundle);
  op_map[AST::Expr::OpBinary::Neq] = ops_binary::builder_ne(bundle);

  op_map[AST::Expr::OpBinary::Gt] = ops_binary::builder_gt(bundle);
  op_map[AST::Expr::OpBinary::Lt] = ops_binary::builder_lt(bundle);

  op_map[AST::Expr::OpBinary::Geq] = ops_binary::builder_ge(bundle);
  op_map[AST::Expr::OpBinary::Leq] = ops_binary::builder_le(bundle);

  op_map[AST::Expr::OpBinary::And] = ops_binary::builder_and(bundle);
  op_map[AST::Expr::OpBinary::Or] = ops_binary::builder_or(bundle);
}
