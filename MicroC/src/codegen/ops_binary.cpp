#include "LLVMBundle.hpp"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <map>

namespace ops_binary {

OpBinary builder_add(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    return bundle.builder.CreateAdd(a_val, b_val, "op.add");
  };
  return builder;
}

OpBinary builder_sub(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    return bundle.builder.CreateSub(a_val, b_val, "op.sub");
  };
  return builder;
}

OpBinary builder_mul(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    return bundle.builder.CreateMul(a_val, b_val, "op.mul");
  };
  return builder;
}

OpBinary builder_div(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    return bundle.builder.CreateSDiv(a_val, b_val, "op.div");
  };
  return builder;
}

OpBinary builder_mod(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    return bundle.builder.CreateSRem(a_val, b_val, "op.mod");
  };
  return builder;
}

OpBinary builder_eq(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_EQ, a_val, b_val);
  };
  return builder;
}

OpBinary builder_ne(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_NE, a_val, b_val);
  };
  return builder;
}

OpBinary builder_gt(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SGT, a_val, b_val);
  };
  return builder;
}

OpBinary builder_lt(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SLT, a_val, b_val);
  };
  return builder;
}

OpBinary builder_ge(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SGE, a_val, b_val);
  };
  return builder;
}

OpBinary builder_le(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SLE, a_val, b_val);
  };
  return builder;
}

OpBinary builder_and(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    return bundle.builder.CreateAnd(a_val, b_val);
  };
  return builder;
}

OpBinary builder_or(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](AST::ExprHandle a, AST::ExprHandle b) {
    llvm::Value *a_val = a->codegen(bundle);
    llvm::Value *b_val = b->codegen(bundle);

    return bundle.builder.CreateOr(a_val, b_val);
  };
  return builder;
}

} // namespace ops_binary
void extend_ops_binary(LLVMBundle &bundle, OpsBinaryMap &op_map) {

  op_map["+"] = ops_binary::builder_add(bundle);
  op_map["-"] = ops_binary::builder_sub(bundle);
  op_map["*"] = ops_binary::builder_mul(bundle);
  op_map["/"] = ops_binary::builder_div(bundle);
  op_map["%"] = ops_binary::builder_mod(bundle);

  op_map["=="] = ops_binary::builder_eq(bundle);
  op_map["!="] = ops_binary::builder_ne(bundle);

  op_map[">"] = ops_binary::builder_gt(bundle);
  op_map["<"] = ops_binary::builder_lt(bundle);

  op_map[">="] = ops_binary::builder_ge(bundle);
  op_map["<="] = ops_binary::builder_le(bundle);

  op_map["&&"] = ops_binary::builder_and(bundle);
  op_map["||"] = ops_binary::builder_or(bundle);
}
