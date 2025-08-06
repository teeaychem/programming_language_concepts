%skeleton "lalr1.cc"
%require "3.8.2"
%header

%define api.token.raw

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
  #include <string>
  #include <tuple>

  #include "AST/AST.hpp"
  #include "AST/Block.hpp"  

  struct Driver;
}

%param { Driver& driver } // Parsing context

%locations

%define parse.trace
%define parse.error detailed
%define parse.lac full

%code {

#include <tuple>

#include "Driver.hpp"

#include "AST/Node/Dec.hpp"
#include "AST/Node/Expr.hpp"
#include "AST/Node/Stmt.hpp"

#include "AST/Types.hpp"

}

%define api.token.prefix {TOK_}

%token <int> CSTINT CSTBOOL
%token <std::string> CSTSTRING NAME

%token
  CHAR ELSE IF INT NULL PRINT PRINTLN RETURN VOID WHILE FOR
   COLON QMARK
   INC DEC
   PLUS MINUS STAR SLASH MOD
   PLUS_ASSIGN MINUS_ASSIGN STAR_ASSIGN SLASH_ASSIGN MOD_ASSIGN
   EQ NE GT LT GE LE
   NOT SEQOR SEQAND
   LPAR RPAR LBRACE RBRACE LBRACK RBRACK SEMI COMMA ASSIGN AMP
;


%right ASSIGN             /* lowest precedence */
%nonassoc PRINT
%left QMARK COLON
%left SEQOR
%left SEQAND
%left EQ NE
%nonassoc GT LT GE LE
%left PLUS_ASSIGN MINUS_ASSIGN
%left PLUS MINUS
%left INC DEC
%left STAR_ASSIGN SLASH_ASSIGN MOD_ASSIGN
%left STAR SLASH MOD
%nonassoc NOT AMP
%nonassoc LBRACK          /* highest precedence  */


%nterm program

%nterm Topdecs
%nterm Topdec

%nterm <AST::DecHandle> Fndec

%nterm <AST::ArgVec> Paramdecs
%nterm <AST::ArgVec> ParamdecsNE

%nterm <AST::StmtBlockHandle> Block
%nterm <AST::Block> StmtOrDecSeq

%nterm <AST::StmtHandle> Stmt
%nterm <AST::StmtHandle> StmtA
%nterm <AST::StmtHandle> StmtB

%nterm <std::pair<std::string, AST::TypHandle>> Vardec
%nterm <std::pair<std::string, AST::TypHandle>> Vardesc

%nterm <AST::ExprHandle> AtomicConst
%nterm <AST::ExprHandle> Expr

%nterm <std::vector<AST::ExprHandle>> Exprs
%nterm <std::vector<AST::ExprHandle>> ExprsNE

%nterm <AST::TypHandle> DataType

%nterm <AST::PrototypeHandle> FnPrototype

%printer {  } <*>; // yyo << $$;

%%
%start program;
program:
    Topdecs YYEOF  {   }
;



Block:
    LBRACE StmtOrDecSeq RBRACE  { $2.finalize(driver.llvm.env_ast); $$ = driver.pk_StmtBlock(std::move($2)); }
;


AtomicConst:
    CSTINT        { $$ = driver.pk_ExprCstI($1);                                              }
  | CSTBOOL       { $$ = std::move(driver.pk_ExprCstI($1));                                   }
  | NULL          { $$ = driver.pk_ExprPrim1(AST::Expr::OpUnary::Sub, driver.pk_ExprCstI(1)); }
;


DataType:
    INT   { $$ = AST::Typ::pk_Int();  }
  | CHAR  { $$ = AST::Typ::pk_Char(); }
;



Exprs:
    %empty   { $$ = std::vector<AST::ExprHandle>(); }
  | ExprsNE  { $$ = $1;                             }
;  


ExprsNE:
    Expr                { std::vector<AST::ExprHandle> es{$1}; $$ = es; }
  | ExprsNE COMMA Expr  { $1.push_back(std::move($3));         $$ = $1; }
;  


Expr:
    NAME                      { $$ = driver.pk_ExprVar($1);                                       }
  | AtomicConst               { $$ = $1;                                                          }
  | Expr ASSIGN Expr          { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::Assign, $1, $3);    }
  | Expr PLUS_ASSIGN Expr     { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::AssignAdd, $1, $3); }
  | Expr MINUS_ASSIGN Expr    { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::AssignSub, $1, $3); }
  | Expr STAR_ASSIGN Expr     { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::AssignMul, $1, $3); }
  | Expr SLASH_ASSIGN Expr    { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::AssignDiv, $1, $3); }
  | Expr MOD_ASSIGN Expr      { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::AssignMod, $1, $3); }
  | NAME LPAR Exprs RPAR      { $$ = driver.pk_ExprCall($1, $3);                                  }
  | MINUS Expr                { $$ = driver.pk_ExprPrim1(AST::Expr::OpUnary::Sub, $2);          }
  | AMP Expr                  { $$ = driver.pk_ExprPrim1(AST::Expr::OpUnary::AddressOf, $2);      }
  | STAR Expr                 { $$ = driver.pk_ExprPrim1(AST::Expr::OpUnary::Dereference, $2);    }
  | NOT Expr                  { $$ = driver.pk_ExprPrim1(AST::Expr::OpUnary::Negation, $2);       }
  | PRINT Expr                { $$ = driver.pk_ExprCall("printi", $2);                            }
  | PRINTLN                   { $$ = driver.pk_ExprCall("println");                               }
  | Expr PLUS  Expr           { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::Add,  $1, $3);      }
  | Expr MINUS Expr           { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::Sub,  $1, $3);      }
  | Expr STAR  Expr           { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::Mul,  $1, $3);      }
  | Expr SLASH Expr           { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::Div,  $1, $3);      }
  | Expr MOD   Expr           { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::Mod,  $1, $3);      }
  | Expr EQ    Expr           { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::Eq, $1, $3);        }
  | Expr NE    Expr           { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::Neq, $1, $3);       }
  | Expr GT    Expr           { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::Gt,  $1, $3);       }
  | Expr LT    Expr           { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::Lt,  $1, $3);       }
  | Expr GE    Expr           { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::Geq, $1, $3);       }
  | Expr LE    Expr           { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::Leq, $1, $3);       }
  | Expr SEQAND Expr          { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::And, $1, $3);       }
  | Expr SEQOR  Expr          { $$ = driver.pk_ExprPrim2(AST::Expr::OpBinary::Or, $1, $3);        }
  | Expr LBRACK Expr RBRACK   { $$ = driver.pk_ExprIndex($1, $3);                                 }
  | LPAR Expr RPAR            { $$ = $2;                                                          }  
;


FnPrototype:
    VOID NAME LPAR Paramdecs RPAR      {
      driver.add_to_env($4);
      $$ = driver.pk_Prototype(AST::Typ::pk_Void(), $2, $4); }
  | DataType NAME LPAR Paramdecs RPAR  {
      driver.add_to_env($4);
      $$ = driver.pk_Prototype($1, $2, $4);                  }
;

Fndec:
    FnPrototype Block      {
      auto r = driver.pk_DecFn($1, $2);
      driver.fn_finalise(r);
      $$ = r;
    }
;


Paramdecs:
    %empty       { $$ = AST::ArgVec{}; }
  | ParamdecsNE  { $$ = $1;              }
;


ParamdecsNE:
    Vardec                    { $$ = AST::ArgVec{$1};    }
  | ParamdecsNE COMMA Vardec  { $1.push_back($3); $$ = $1; }
;


Stmt:
    StmtA  { $$ = $1; }
  | StmtB  { $$ = $1; }
;


StmtA:  /* No unbalanced if-else */
    Expr SEMI                           { $$ = driver.pk_StmtExpr($1);                   }
  | RETURN SEMI                         { $$ = driver.pk_StmtReturn(std::nullopt);       }
  | RETURN Expr SEMI                    { $$ = driver.pk_StmtReturn($2);                 }
  | Block                               { $$ = std::static_pointer_cast<AST::StmtT>($1); }
  | IF LPAR Expr RPAR StmtA ELSE StmtA  { $$ = driver.pk_StmtIf($3, $5, $7);             }
  | WHILE LPAR Expr RPAR StmtA          { $$ = driver.pk_StmtWhile($3, $5);              }
;


StmtB:
    IF LPAR Expr RPAR StmtA ELSE StmtB  { $$ = driver.pk_StmtIf($3, $5, $7); }
  | IF LPAR Expr RPAR Stmt              {
      auto empty_block = driver.pk_StmtBlockStmt(AST::Block{});
      $$ = driver.pk_StmtIf($3, $5, empty_block);                            }
  | WHILE LPAR Expr RPAR StmtB          { $$ = driver.pk_StmtWhile($3, $5);  }
;


StmtOrDecSeq:
    %empty                    { $$ = AST::Block{};                              }
  | StmtOrDecSeq Stmt         { $1.push_Stmt($2); $$ = $1;                      }
  | StmtOrDecSeq Vardec SEMI  {
      auto dec = driver.pk_DecVar(AST::Dec::Scope::Local, $2.second, $2.first);
      auto s = driver.pk_StmtDeclaration(dec);
      $$ = $1.push_DecVar(driver.llvm.env_ast, s);                              }
;


Topdecs:
    %empty          {  }
  | Topdec Topdecs  {  }
;


Topdec:
   Vardec SEMI  {
     auto dec = driver.pk_DecVar(AST::Dec::Scope::Global, $1.second, $1.first);
     auto s = driver.pk_StmtDeclaration(dec);
     driver.push_dec(s);                                                        }
  | Fndec       {
    auto s = driver.pk_StmtDeclaration(std::move($1));
    driver.push_dec(s);                                                         }
;


Vardec:
    DataType Vardesc  { $$ = std::make_pair($2.first, $2.second->complete_with($1)); }
;


Vardesc:
    NAME                          { $$ = std::make_pair($1, AST::Typ::pk_Void());                             }
  | STAR Vardesc                  { $$ = std::make_pair($2.first, AST::Typ::pk_Ptr($2.second, std::nullopt)); }
  | LPAR Vardesc RPAR             { $$ = $2;                                                                  }
  | Vardesc LBRACK RBRACK         { $$ = std::make_pair($1.first, AST::Typ::pk_Ptr($1.second, std::nullopt)); }
  | Vardesc LBRACK CSTINT RBRACK  { $$ = std::make_pair($1.first, AST::Typ::pk_Ptr($1.second, $3));           }
;


%%


void yy::parser::error (const location_type& l, const std::string& m) {
  std::cerr << l << ": " << m << '\n';
}
