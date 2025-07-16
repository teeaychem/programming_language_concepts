#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include "AST/AST.hh"

namespace AST {

namespace Typ {

// TypData

struct TypData : TypT {
  Data d_typ;

  TypData(Data d_typ)
      : d_typ(d_typ) {};

  Typ::Kind kind() const override { return Typ::Kind::Data; }
  std::string to_string(size_t indent) const override;

  void complete_data(Data d_typ) override {
    if (d_typ == Data::Void) {
      throw std::logic_error("Attempt to complete type with void.");
    }

    if (this->d_typ == Data::Void) {
      this->d_typ = d_typ;
    } else {
      throw std::logic_error("Attempt to complete a completed type.");
    }
  }

  TypHandle deref() const override {
    std::cerr << "deref() called on TypData" << std::endl;
    exit(-1);
  }

  llvm::Type *typegen(LLVMBundle &hdl) override {
    switch (d_typ) {
    case Data::Int:
      return llvm::Type::getInt64Ty(*hdl.context);
    case Data::Char:
      return llvm::Type::getInt8Ty(*hdl.context);
    case Data::Void:
      return llvm::Type::getVoidTy(*hdl.context);
    }
  }
};

inline TypHandle pk_Data(Data d_typ) {
  TypData d{d_typ};
  return std::make_shared<TypData>(std::move(d));
}

inline TypHandle pk_Void() {
  TypData d{Data::Void};
  return std::make_shared<TypData>(std::move(d));
}

// TypArr

struct TypIndex : TypT {
  TypHandle typ;
  std::optional<std::size_t> size;

  TypIndex(TypHandle typ, std::optional<std::int64_t> size)
      : typ(std::move(typ)), size(size) {}

  Typ::Kind kind() const override { return Typ::Kind::Arr; }
  std::string to_string(size_t indent) const override;
  TypHandle expr_type() const {
    return typ;
  }
  std::optional<std::size_t> type_size() const {
    return size;
  }

  void complete_data(Data d_typ) override { typ->complete_data(d_typ); }

  TypHandle deref() const override {
    return typ;
  }

  llvm::Type *typegen(LLVMBundle &hdl) override {
    return llvm::ArrayType::get(typ->typegen(hdl), size.value_or(0));
  }
};

inline TypHandle pk_Arr(TypHandle typ, std::optional<std::int64_t> size) {
  TypIndex t(std::move(typ), std::move(size));
  return std::make_shared<TypIndex>(std::move(t));
}

inline TypHandle pk_Arr() {
  TypIndex t(std::move(pk_Void()), std::nullopt);
  return std::make_shared<TypIndex>(std::move(t));
}

// TypPtr

struct TypPtr : TypT {
  TypHandle dest;

  TypPtr(TypHandle of) : dest(std::move(of)) {}

  Typ::Kind kind() const override { return Typ::Kind::Ptr; }
  std::string to_string(size_t indent) const override;

  void complete_data(Data d_typ) override { dest->complete_data(d_typ); }

  TypHandle deref() const override {
    return dest;
  }

  llvm::Type *typegen(LLVMBundle &hdl) override {
    return llvm::PointerType::getUnqual(*hdl.context);
  }
};

inline TypHandle pk_Ptr(TypHandle of) {
  TypPtr t(std::move(of));
  return std::make_shared<TypPtr>(std::move(t));
}

inline TypHandle pk_Ptr() {
  TypPtr t{std::move(pk_Void())};
  return std::make_shared<TypPtr>(std::move(t));
}

} // namespace Typ

} // namespace AST
