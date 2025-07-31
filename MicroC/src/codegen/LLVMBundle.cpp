#include "LLVMBundle.hpp"
#include "AST/AST.hpp"

llvm::Value *LLVMBundle::ensure_loaded(AST::TypHandle typ, llvm::Value *value) {

  if (typ == nullptr) {
    throw std::logic_error("Unable to load without type information");
  }

  switch (typ->kind()) {

  case AST::Typ::Kind::Ptr: {
    // FIXME: Fresh take likely needed.
    // Something else is needed to ensure pointers to pointers are fine.
    // Inspection of AST types should be sufficient.
    if (!value->getType()->isPointerTy()) {
      throw std::logic_error("Attempt to load non-pointer into pointer.");
    }
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
  }
}
