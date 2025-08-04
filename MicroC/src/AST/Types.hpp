#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include "AST/AST.hpp"
#include "codegen/LLVMBundle.hpp"

namespace AST {

namespace Typ {

// TypData

struct Int : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Int; }

  Int() {};

  std::string to_string(size_t indent = 0) const override;

  TypHandle deref() const override { throw std::logic_error(std::format("deref called on an int")); }

  TypHandle complete_data(TypHandle data) override { throw std::logic_error("Complete into int."); }

  llvm::Type *
  llvm(LLVMBundle &hdl) const override { return llvm::Type::getInt64Ty(*hdl.context); }

  // The default value for a type, used during declarations, etc.
  // Throws on void type.
  llvm::Constant *defaultgen(LLVMBundle &hdl) const {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*hdl.context), 0);
  }
};

struct Char : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Char; }

  Char() {};

  std::string to_string(size_t indent = 0) const override;
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

struct Void : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Void; }

  Void() {};

  std::string to_string(size_t indent = 0) const override;

  TypHandle deref() const override { throw std::logic_error("deref() called on void"); }

  TypHandle complete_data(TypHandle data) override { return data; }

  llvm::Type *llvm(LLVMBundle &hdl) const override {

    return llvm::Type::getVoidTy(*hdl.context);
  }

  llvm::Constant *defaultgen(LLVMBundle &hdl) const { throw std::logic_error("Declaration of void type"); }
};

// TypIndex

struct Ptr : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Ptr; }

private:
  // What's pointed to.
  TypHandle _pointee;

  // The 'area' of the pointer.
  // If Some(a) then the pointer can be offset a times (incl. zero) to point to some `pointee`.
  std::optional<std::size_t> _area;

public:
  Ptr(TypHandle typ, std::optional<std::int64_t> area)
      : _pointee(std::move(typ)),
        _area(area) {}

  std::string to_string(size_t indent = 0) const override;

  TypHandle pointee_type() const { return _pointee; }
  std::optional<std::size_t> area() const { return _area; }

  TypHandle complete_data(TypHandle data) override {

    switch (this->_pointee->kind()) {

    case Kind::Int:
    case Kind::Char: {
      throw std::logic_error("Idx to data");
    } break;

    case Kind::Void: {
      auto fresh_pointer = Ptr(data, std::nullopt);
      auto fresh_handle = std::make_shared<AST::Typ::Ptr>(fresh_pointer);

      this->_pointee = fresh_handle;
      return fresh_handle;
    } break;

    case Kind::Ptr: {
      auto fresh_destination = this->_pointee->complete_data(data);
      auto fresh_pointer = Ptr(fresh_destination, std::nullopt);
      auto fresh_handle = std::make_shared<AST::Typ::Ptr>(fresh_pointer);

      this->_pointee = fresh_destination;
      return fresh_destination;
    } break;
    }
  }

  TypHandle deref() const override { return _pointee; }

  llvm::Type *llvm(LLVMBundle &hdl) const override {

    if (this->_area.has_value()) {
      return llvm::ArrayType::get(this->_pointee->llvm(hdl), this->_area.value());
    } else {
      return llvm::PointerType::getUnqual(*hdl.context);
    }
  }
};

} // namespace Typ

} // namespace AST
