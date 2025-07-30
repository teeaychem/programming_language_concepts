#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include "AST/AST.hpp"
#include "codegen/LLVMBundle.hpp"

namespace AST {

namespace Typ {

// TypData

struct TypData : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Data; }

  Data data;

  TypData(Data data)
      : data(data) {};

  std::string to_string(size_t indent) const override;
  TypHandle deref_unsafe() const override { throw std::logic_error("deref() called on TypData"); }

  // Base case of `complete_data_unsafe`, replaces void with `data` or throws a logic_error.
  void complete_data_unsafe(Data data) override {
    if (data == Data::Void) {
      throw std::logic_error("Attempt to complete type with void.");
    }

    if (this->data == Data::Void) {
      this->data = data;
    } else {
      throw std::logic_error("Attempt to complete a completed type.");
    }
  }

  llvm::Type *typegen(LLVMBundle &hdl) const override {
    switch (data) {
    case Data::Int:
      return llvm::Type::getInt64Ty(*hdl.context);
    case Data::Char:
      return llvm::Type::getInt8Ty(*hdl.context);
    case Data::Void:
      return llvm::Type::getVoidTy(*hdl.context);
    }
  }

  // The default value for a type, used during declarations, etc.
  // Throws on void type.
  llvm::Constant *defaultgen(LLVMBundle &hdl) const {
    switch (data) {
    case Data::Int:
      return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*hdl.context), 0);
    case Data::Char:
      return llvm::ConstantInt::get(llvm::Type::getInt8Ty(*hdl.context), 0);
    case Data::Void:
      throw std::logic_error("Declaration of void type");
    }
  }
};

inline TypHandle pk_Data(Data data) {
  TypData type_data{data};
  return std::make_shared<TypData>(std::move(type_data));
}

inline TypHandle pk_Void() {
  TypData type_data{Data::Void};
  return std::make_shared<TypData>(std::move(type_data));
}

// TypArr

struct TypIndex : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Array; }

  TypHandle typ;
  std::optional<std::size_t> size;

  TypIndex(TypHandle typ, std::optional<std::int64_t> size)
      : typ(std::move(typ)), size(size) {}

  std::string to_string(size_t indent) const override;
  TypHandle expr_type() const { return typ; }
  std::optional<std::size_t> type_size() const { return size; }
  void complete_data_unsafe(Data data) override { typ->complete_data_unsafe(data); }
  TypHandle deref_unsafe() const override { return typ; }

  llvm::Type *typegen(LLVMBundle &hdl) const override {
    return llvm::ArrayType::get(typ->typegen(hdl), size.value_or(0));
  }
};

inline TypHandle pk_Index(TypHandle typ, std::optional<std::int64_t> size) {
  TypIndex type_index(std::move(typ), std::move(size));
  return std::make_shared<TypIndex>(std::move(type_index));
}

// TypPtr

struct TypPointer : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Pointer; }

  TypHandle destination;

  TypPointer(TypHandle of) : destination(std::move(of)) {}

  std::string to_string(size_t indent) const override;
  void complete_data_unsafe(Data data) override { destination->complete_data_unsafe(data); }
  TypHandle deref_unsafe() const override { return destination; }
  llvm::Type *typegen(LLVMBundle &hdl) const override { return llvm::PointerType::getUnqual(*hdl.context); }

  // An opaque pointer, given LLVMs preference for these.
  llvm::Constant *defaultgen(LLVMBundle &hdl) const {
    return llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*hdl.context));
  }
};

inline TypHandle pk_Ptr(TypHandle of) {
  TypPointer type_pointer(std::move(of));
  return std::make_shared<TypPointer>(std::move(type_pointer));
}

} // namespace Typ

} // namespace AST
