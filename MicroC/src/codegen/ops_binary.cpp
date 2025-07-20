#include "LLVMBundle.hpp"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <map>

namespace ops_binary {

OpBinary builder_add(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](llvm::Value *LHS, llvm::Value *RHS) {
    return bundle.builder.CreateAdd(LHS, RHS, "op.add");
  };
  return builder;
}

OpBinary builder_sub(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](llvm::Value *LHS, llvm::Value *RHS) {
    return bundle.builder.CreateSub(LHS, RHS, "op.sub");
  };
  return builder;
}

OpBinary builder_mul(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](llvm::Value *LHS, llvm::Value *RHS) {
    return bundle.builder.CreateMul(LHS, RHS, "op.mul");
  };
  return builder;
}

OpBinary builder_div(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](llvm::Value *LHS, llvm::Value *RHS) {
    return bundle.builder.CreateSDiv(LHS, RHS, "op.div");
  };
  return builder;
}

OpBinary builder_mod(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](llvm::Value *LHS, llvm::Value *RHS) {
    return bundle.builder.CreateSRem(LHS, RHS, "op.mod");
  };
  return builder;
}

OpBinary builder_eq(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](llvm::Value *LHS, llvm::Value *RHS) {
    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_EQ, LHS, RHS);
  };
  return builder;
}

OpBinary builder_ne(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](llvm::Value *LHS, llvm::Value *RHS) {
    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_NE, LHS, RHS);
  };
  return builder;
}

OpBinary builder_gt(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](llvm::Value *LHS, llvm::Value *RHS) {
    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SGT, LHS, RHS);
  };
  return builder;
}

OpBinary builder_lt(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](llvm::Value *LHS, llvm::Value *RHS) {
    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SLT, LHS, RHS);
  };
  return builder;
}

OpBinary builder_ge(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](llvm::Value *LHS, llvm::Value *RHS) {
    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SGE, LHS, RHS);
  };
  return builder;
}

OpBinary builder_le(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](llvm::Value *LHS, llvm::Value *RHS) {
    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SLE, LHS, RHS);
  };
  return builder;
}

OpBinary builder_and(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](llvm::Value *LHS, llvm::Value *RHS) {
    return bundle.builder.CreateAnd(LHS, RHS);
  };
  return builder;
}

OpBinary builder_or(LLVMBundle &bundle) {
  OpBinary builder = [&bundle](llvm::Value *LHS, llvm::Value *RHS) {
    return bundle.builder.CreateOr(LHS, RHS);
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
