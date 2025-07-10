#pragma once

#include <cstdint>
#include <fmt/base.h>
#include <fmt/format.h>
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
  std::string to_string() const override;

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

  const TypData *as_TypData() const & { return this; }
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

struct TypArr : TypT {
  TypHandle typ;
  std::optional<std::int64_t> size;

  TypArr(TypHandle typ, std::optional<std::int64_t> size)
      : typ(std::move(typ)), size(size) {}

  Typ::Kind kind() const override { return Typ::Kind::Arr; }
  std::string to_string() const override;

  void complete_data(Data d_typ) override { typ->complete_data(d_typ); }

  llvm::Type *typegen(LLVMBundle &hdl) override {
    return llvm::ArrayType::get(typ->typegen(hdl), size.value_or(0));
  }
};

inline TypHandle pk_Arr(TypHandle typ, std::optional<std::int64_t> size) {
  TypArr t(std::move(typ), std::move(size));
  return std::make_shared<TypArr>(std::move(t));
}

inline TypHandle pk_Arr() {
  TypArr t(std::move(pk_Void()), std::nullopt);
  return std::make_shared<TypArr>(std::move(t));
}

// TypPtr

struct TypPtr : TypT {
  TypHandle dest;

  TypPtr(TypHandle of) : dest(std::move(of)) {}

  Typ::Kind kind() const override { return Typ::Kind::Ptr; }
  std::string to_string() const override;

  void complete_data(Data d_typ) override { dest->complete_data(d_typ); }
  llvm::Type *typegen(LLVMBundle &hdl) override {
    return llvm::PointerType::getUnqual(*hdl.Context);
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
