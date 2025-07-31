#include "LLVMBundle.hpp"
#include "AST/AST.hpp"
#include "AST/Types.hpp"

#include <iostream>
#include <stdexcept>

llvm::Value *LLVMBundle::ensure_loaded(AST::TypHandle typ, llvm::Value *value) {

  if (typ == nullptr) {
    throw std::logic_error("Unable to load without type information");
  }

  switch (typ->kind()) {

  case AST::Typ::Kind::Array: {
    // FIXME
    return value;
  } break;

  case AST::Typ::Kind::Int: {
    if (value->getType()->isPointerTy()) {
      return this->builder.CreateLoad(typ->llvm(*this), value);
    } else {
      return value;
    }
  }

  case AST::Typ::Kind::Char: {
    if (value->getType()->isPointerTy()) {
      return this->builder.CreateLoad(typ->llvm(*this), value);
    } else {
      return value;
    }
  }

  case AST::Typ::Kind::Void: {
    return value;

  } break;

  case AST::Typ::Kind::Pointer: {
    // FIXME
    return value;
  } break;
  }
}
