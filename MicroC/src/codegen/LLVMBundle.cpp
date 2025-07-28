#include "LLVMBundle.hpp"
#include "AST/AST.hpp"
#include "AST/Types.hpp"

#include <iostream>

llvm::Value *LLVMBundle::ensure_loaded(AST::TypHandle typ, llvm::Value *value) {
  std::cout << "ensure_loaded called" << "\n";
  switch (typ->kind()) {

  case AST::Typ::Kind::Array: {
    // FIXME
    return value;
  } break;

  case AST::Typ::Kind::Data: {

    if (value->getType()->isPointerTy()) {
      return this->builder.CreateLoad(typ->typegen(*this), value);
    } else {
      return value;
    }

  } break;

  case AST::Typ::Kind::Pointer: {
    // FIXME
    return value;
  } break;
  }
}
