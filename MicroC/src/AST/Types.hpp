#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include "AST/AST.hpp"
#include "codegen/LLVMBundle.hpp"

namespace AST {

namespace Typ {

// TypData

struct TypInt : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Int; }

  TypInt() {};

  std::string to_string(size_t indent) const override;
  TypHandle deref() const override { throw std::logic_error("deref called on an int"); }

  TypHandle complete_data(TypHandle data) override { throw std::logic_error("Complete into int."); }

  llvm::Type *
  llvm(LLVMBundle &hdl) const override { return llvm::Type::getInt64Ty(*hdl.context); }

  // The default value for a type, used during declarations, etc.
  // Throws on void type.
  llvm::Constant *defaultgen(LLVMBundle &hdl) const {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*hdl.context), 0);
  }
};

struct TypChar : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Char; }

  TypChar() {};

  std::string to_string(size_t indent) const override;
  TypHandle deref() const override { throw std::logic_error("deref called on a char"); }

  TypHandle complete_data(TypHandle data) override { throw std::logic_error("Complete into char."); }

  llvm::Type *
  llvm(LLVMBundle &hdl) const override { return llvm::Type::getInt8Ty(*hdl.context); }

  // The default value for a type, used during declarations, etc.
  // Throws on void type.
  llvm::Constant *defaultgen(LLVMBundle &hdl) const {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(*hdl.context), 0);
  }
};

struct TypVoid : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Void; }

  TypVoid() {};

  std::string to_string(size_t indent) const override;

  TypHandle deref() const override { throw std::logic_error("deref() called on TypVoid"); }

  TypHandle complete_data(TypHandle data) override { return data; }

  llvm::Type *llvm(LLVMBundle &hdl) const override {

    return llvm::Type::getVoidTy(*hdl.context);
  }

  llvm::Constant *defaultgen(LLVMBundle &hdl) const { throw std::logic_error("Declaration of void type"); }
};

// TypPtr

struct TypPointer : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Pointer; }

  TypHandle destination;

  TypPointer(TypHandle of) : destination(std::move(of)) {}

  std::string to_string(size_t indent) const override;

  TypHandle complete_data(TypHandle data) override {

    switch (this->destination->kind()) {

    case Kind::Int:
    case Kind::Char: {
      throw std::logic_error("Idx to data");
    } break;
    case Kind::Void: {
      auto fresh_pointer = TypPointer(data);
      auto fresh_handle = std::make_shared<AST::Typ::TypPointer>(fresh_pointer);

      this->destination = fresh_handle;
      return fresh_handle;
    } break;
    case Kind::Array:
    case Kind::Pointer: {
      auto fresh_destination = destination->complete_data(data);
      auto fresh_pointer = TypPointer(fresh_destination);
      auto fresh_handle = std::make_shared<AST::Typ::TypPointer>(fresh_pointer);

      this->destination = fresh_destination;
      return fresh_destination;
    } break;
    }
  }

  TypHandle deref() const override { return destination; }
  llvm::Type *llvm(LLVMBundle &hdl) const override { return llvm::PointerType::getUnqual(*hdl.context); }

  // An opaque pointer, given LLVMs preference for these.
  llvm::Constant *defaultgen(LLVMBundle &hdl) const {
    return llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*hdl.context));
  }
};

// TypArr

struct TypIndex : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Array; }

  TypHandle destination;
  std::optional<std::size_t> size;

  TypIndex(TypHandle typ, std::optional<std::int64_t> size)
      : destination(std::move(typ)), size(size) {}

  std::string to_string(size_t indent) const override;
  TypHandle expr_type() const { return destination; }
  std::optional<std::size_t> type_size() const { return size; }

  TypHandle complete_data(TypHandle data) override {

    switch (this->destination->kind()) {

    case Kind::Int:
    case Kind::Char: {
      throw std::logic_error("Idx to data");
    } break;
    case Kind::Void: {
      auto fresh_pointer = TypPointer(data);
      auto fresh_handle = std::make_shared<AST::Typ::TypPointer>(fresh_pointer);

      this->destination = fresh_handle;
      return fresh_handle;
    } break;
    case Kind::Array:
    case Kind::Pointer: {
      auto fresh_destination = destination->complete_data(data);
      auto fresh_pointer = TypPointer(fresh_destination);
      auto fresh_handle = std::make_shared<AST::Typ::TypPointer>(fresh_pointer);

      this->destination = fresh_destination;
      return fresh_destination;
    } break;
    }
  }

  TypHandle deref() const override { return destination; }

  llvm::Type *llvm(LLVMBundle &hdl) const override {
    return llvm::ArrayType::get(destination->llvm(hdl), size.value_or(0));
  }
};

} // namespace Typ

} // namespace AST
