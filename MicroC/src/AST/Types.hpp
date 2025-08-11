#pragma once

#include <string>

#include "AST/AST.hpp"
#include "codegen/Structs.hpp"

namespace AST {

namespace Typ {

struct Bool : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Bool; }

  Bool() {};

  std::string to_string(size_t indent = 0) const override;
  TypHandle deref() const override { throw std::logic_error("deref called on a bool"); }

  TypHandle complete_with(TypHandle data) override { throw std::logic_error("Complete into bool"); }

  llvm::Type *codegen(Context &ctx) const override { return ctx.get_typ(this->kind()); }
};

struct Char : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Char; }

  Char() {};

  std::string to_string(size_t indent = 0) const override;

  TypHandle deref() const override { throw std::logic_error("deref called on a char"); }

  TypHandle complete_with(TypHandle data) override { throw std::logic_error("Complete into char."); }

  llvm::Type *codegen(Context &ctx) const override { return ctx.get_typ(this->kind()); }
};

struct Int : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Int; }

  Int() {};

  std::string to_string(size_t indent = 0) const override;

  TypHandle deref() const override { throw std::logic_error("deref called on an int"); }

  TypHandle complete_with(TypHandle data) override { throw std::logic_error("Complete into int."); }

  llvm::Type *codegen(Context &ctx) const override { return ctx.get_typ(this->kind()); }
};

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
      : _pointee(typ),
        _area(area) {
  }

  std::string to_string(size_t indent = 0) const override;

  TypHandle pointee_type() const { return _pointee; }
  std::optional<std::size_t> area() const { return _area; }

  TypHandle complete_with(TypHandle data) override {

    switch (this->_pointee->kind()) {

    case Kind::Bool:
    case Kind::Char:
    case Kind::Int: {
      throw std::logic_error("Idx to data type");
    } break;

    case Kind::Ptr: {
      auto pointee_ptr = std::static_pointer_cast<AST::Typ::Ptr>(this->_pointee);

      auto fresh_destination = this->_pointee->complete_with(data);
      auto fresh_pointer = Ptr(fresh_destination, this->_area);
      auto fresh_handle = std::make_shared<AST::Typ::Ptr>(fresh_pointer);

      return fresh_handle;
    } break;

    case Kind::Void: {

      auto fresh_pointer = Ptr(data, this->_area);
      auto fresh_handle = std::make_shared<AST::Typ::Ptr>(fresh_pointer);

      return fresh_handle;
    } break;
    }
  }

  TypHandle deref() const override { return _pointee; }

  llvm::Type *codegen(Context &ctx) const override {

    if (this->_area.has_value()) {
      return llvm::ArrayType::get(this->_pointee->codegen(ctx), this->_area.value());
    } else {
      return ctx.get_typ(this->kind());
    }
  }
};

struct Void : TypT {
  Typ::Kind kind() const override { return Typ::Kind::Void; }

  Void() {};

  std::string to_string(size_t indent = 0) const override;

  TypHandle deref() const override { throw std::logic_error("deref() called on void"); }

  TypHandle complete_with(TypHandle data) override { return data; }

  llvm::Type *codegen(Context &ctx) const override { return ctx.get_typ(this->kind()); }
};

// pk typ

inline AST::TypHandle pk_Bool() {
  AST::Typ::Bool typ_bool{};
  return std::make_shared<AST::Typ::Bool>(typ_bool);
};

inline AST::TypHandle pk_Char() {
  AST::Typ::Char type_char{};
  return std::make_shared<AST::Typ::Char>(type_char);
};

inline AST::TypHandle pk_Int() {
  AST::Typ::Int type_int{};
  return std::make_shared<AST::Typ::Int>(type_int);
};

inline AST::TypHandle pk_Ptr(AST::TypHandle typ, std::optional<std::int64_t> area) {
  AST::Typ::Ptr type_ptr(typ, area);
  return std::make_shared<AST::Typ::Ptr>(type_ptr);
}

inline AST::TypHandle pk_Void() {
  AST::Typ::Void type_void{};
  return std::make_shared<AST::Typ::Void>(type_void);
}

} // namespace Typ

} // namespace AST
