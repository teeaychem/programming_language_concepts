#pragma once

#include "AST/AST.hh"
#include "CodegenLLVM.hh"
#include "parser.hh"

#include <string>

// Give flex the prototype of yylex
#define YY_DECL yy::parser::symbol_type yylex(Driver &drv)
YY_DECL; // Declare the prototype for bison

struct Driver {
  Driver();

  std::vector<AST::DecHandle> prg{};

  void push_dec(AST::DecHandle dec) {
    fmt::println("| {}", dec->to_string());
    prg.push_back(dec);
  }

  int parse(const std::string &f); // Run the parser on file F.  Return 0 on success.

  std::string file; // The file to be parsed

  void scan_begin(); // Handling the scanner.
  void scan_end();

  bool trace_parsing;
  bool trace_scanning;

  yy::location location; // Token location.

  // tmp
};
