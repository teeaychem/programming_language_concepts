%skeleton "lalr1.cc"
%require "3.8.2"
%header

%define api.token.raw

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
  #include <string>
  #include "AST/AST.hh"
  #include "AST/Dec.hh"
  #include "AST/Expr.hh"
  class Driver;
}

%param { Driver& drv } // Parsing context

%locations

%define parse.trace
%define parse.error detailed
%define parse.lac full

%code {
#include "driver.hh"

#include "AST/AST.hh"
#include "AST/Access.hh"
#include "AST/Expr.hh"
#include "AST/Stmt.hh"
#include "AST/Typ.hh"

AST::ExprHandle AccessAssign(std::string op, AST::AccessHandle dest, AST::ExprHandle expr);
AST::ExprHandle char_nl = AST::Expr::pk_CstI(10);
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

%nterm <AST::ParamVec> Paramdecs
%nterm <AST::ParamVec> ParamdecsNE

%nterm <AST::BlockVec> Block
%nterm <AST::BlockVec> StmtOrDecSeq

%nterm <AST::StmtHandle> Stmt
%nterm <AST::StmtHandle> StmtA
%nterm <AST::StmtHandle> StmtB

%nterm <std::pair<AST::TypHandle, std::string>> Vardec
%nterm <std::pair<AST::TypHandle, std::string>> Vardesc

%nterm <AST::ExprHandle> Const
%nterm <AST::ExprHandle> Expr
%nterm <AST::ExprHandle> ExprNotAccess
%nterm <AST::ExprHandle> AtExprNotAccess

%nterm <std::vector<AST::ExprHandle>> Exprs
%nterm <std::vector<AST::ExprHandle>> ExprsNE

%nterm <AST::AccessHandle> Access

%nterm <AST::Typ::Data> DataType

%printer {  } <*>; // yyo << $$;

%%
%start program;
program:
    Topdecs YYEOF  {   }
;


Access:
    NAME                       { $$ = AST::Access::pk_Var($1);              }
  | LPAR Access RPAR           { $$ = $2;                                   }
  | STAR Access                {
      auto acc = AST::Expr::pk_Access(AST::Expr::Access::Mode::Access, $2);
      $$ = AST::Access::pk_Deref(std::move(acc));                           }
  | STAR AtExprNotAccess       { $$ = AST::Access::pk_Deref($2);            }
  | Access LBRACK Expr RBRACK  { $$ = AST::Access::pk_Index($1, $3);        }
;


AtExprNotAccess:
    Const                    { $$ = $1;                                                      }
  | LPAR ExprNotAccess RPAR  { $$ = $2;                                                      }
  | AMP Access               { $$ = AST::Expr::pk_Access(AST::Expr::Access::Mode::Addr, $2); }
;


Block:
    LBRACE StmtOrDecSeq RBRACE  { $$ = $2; }
;


Const:
    CSTINT        { $$ = AST::Expr::pk_CstI($1);                           }
  | CSTBOOL       { $$ = std::move(AST::Expr::pk_CstI($1));                }
  | MINUS CSTINT  { $$ = AST::Expr::pk_Prim1("-", AST::Expr::pk_CstI($2)); }
  | NULL          { $$ = AST::Expr::pk_Prim1("-", AST::Expr::pk_CstI(1));  }
;


DataType:
    INT   { $$ = AST::Typ::Data::Int;  }
  | CHAR  { $$ = AST::Typ::Data::Char; }
;


Expr:
    Access         { $$ = AST::Expr::pk_Access(AST::Expr::Access::Mode::Access, $1); }
  | ExprNotAccess  { $$ = $1;                                                        }
;


Exprs:
    %empty   { $$ = std::vector<AST::ExprHandle>(); }
  | ExprsNE  { $$ = $1;                             }
;  

ExprsNE:
    Expr                { std::vector<AST::ExprHandle> es{$1}; $$ = es; }
  | Expr COMMA ExprsNE  { $3.push_back(std::move($1));         $$ = $3; }
;  


ExprNotAccess:
    AtExprNotAccess           { $$ = $1;                                     }
  | Access ASSIGN Expr        { $$ = AST::Expr::pk_Assign($1, $3);           }
  | Access PLUS_ASSIGN Expr   { $$ = AccessAssign("+", $1, $3);              }
  | Access MINUS_ASSIGN Expr  { $$ = AccessAssign("-", $1, $3);              }
  | Access STAR_ASSIGN Expr   { $$ = AccessAssign("*", $1, $3);              }
  | Access SLASH_ASSIGN Expr  { $$ = AccessAssign("/", $1, $3);              }
  | Access MOD_ASSIGN Expr    { $$ = AccessAssign("%", $1, $3);              }
  | NAME LPAR Exprs RPAR      { $$ = AST::Expr::pk_Call($1, $3);             }
  | NOT Expr                  { $$ = AST::Expr::pk_Prim1("!", $2);           }
  | PRINT Expr                { $$ = AST::Expr::pk_Prim1("printi", $2);      }
  | PRINTLN                   { $$ = AST::Expr::pk_Prim1("printc", char_nl); }
  | Expr PLUS  Expr           { AST::Expr::pk_Prim2("+",  $1, $3);           }
  | Expr MINUS Expr           { AST::Expr::pk_Prim2("-",  $1, $3);           }
  | Expr STAR  Expr           { AST::Expr::pk_Prim2("*",  $1, $3);           }
  | Expr SLASH Expr           { AST::Expr::pk_Prim2("/",  $1, $3);           }
  | Expr MOD   Expr           { AST::Expr::pk_Prim2("%",  $1, $3);           }
  | Expr EQ    Expr           { AST::Expr::pk_Prim2("==", $1, $3);           }
  | Expr NE    Expr           { AST::Expr::pk_Prim2("!=", $1, $3);           }
  | Expr GT    Expr           { AST::Expr::pk_Prim2(">",  $1, $3);           }
  | Expr LT    Expr           { AST::Expr::pk_Prim2("<",  $1, $3);           }
  | Expr GE    Expr           { AST::Expr::pk_Prim2(">=", $1, $3);           }
  | Expr LE    Expr           { AST::Expr::pk_Prim2("<=", $1, $3);           }
;


Fndec:
    VOID NAME LPAR Paramdecs RPAR Block      {
      auto r_typ = AST::Typ::pk_Void();
      auto f = AST::Dec::pk_Fn(r_typ, $2, $4, $6);
      $$ = f;                                     }
  | DataType NAME LPAR Paramdecs RPAR Block  {
      auto r_typ = AST::Typ::pk_Data($1);
      auto f = AST::Dec::pk_Fn(r_typ, $2, $4, $6);
      $$ = f;                                     }
;


Paramdecs:
    %empty       { $$ = AST::ParamVec{}; }
  | ParamdecsNE  { $$ = $1;              }
;

ParamdecsNE:
    Vardec                    { $$ = AST::ParamVec{$1};    }
  | Vardec COMMA ParamdecsNE  { $3.push_back($1); $$ = $3; }
;


Stmt:
    StmtA  { $$ = $1; }
  | StmtB  { $$ = $1; }
;


StmtA:  /* No unbalanced if-else */
    Expr SEMI                           { $$ = AST::Stmt::pk_Expr($1);             }
  | RETURN SEMI                         { $$ = AST::Stmt::pk_Return(std::nullopt); }
  | RETURN Expr SEMI                    { $$ = AST::Stmt::pk_Return($2);           }
  | Block                               { $$ = AST::Stmt::pk_Block(std::move($1)); }
  | IF LPAR Expr RPAR StmtA ELSE StmtA  { $$ = AST::Stmt::pk_If($3, $5, $7);       }
  | WHILE LPAR Expr RPAR StmtA          { $$ = AST::Stmt::pk_While($3, $5);        }
;


StmtB:
    IF LPAR Expr RPAR StmtA ELSE StmtB  { $$ = AST::Stmt::pk_If($3, $5, $7); }
  | IF LPAR Expr RPAR Stmt              {
      auto empty_block = AST::Stmt::pk_Block(AST::BlockVec{});
      $$ = AST::Stmt::pk_If($3, $5, empty_block);                            }
  | WHILE LPAR Expr RPAR StmtB          { $$ = AST::Stmt::pk_While($3, $5);  }
;


StmtOrDecSeq:
    %empty                    { $$ = AST::BlockVec{};                           }
  | Stmt StmtOrDecSeq         { $2.push_back($1); $$ = $2;                      }
  | Vardec SEMI StmtOrDecSeq  {
      auto dec = AST::Dec::pk_Var(AST::Dec::Scope::Local, $1.first, $1.second);
      $3.push_back(dec);
      $$ = $3;                                                                  }
;





Topdecs:
    %empty          {  }
  | Topdec Topdecs  {  }
;


Topdec:
   Vardec SEMI  {
     auto dec = AST::Dec::pk_Var(AST::Dec::Scope::Global, $1.first, $1.second);
     drv.push_dec(dec);                                                         }
  | Fndec       { drv.push_dec($1);                                             }
;


Vardec:
    DataType Vardesc  { $2.first->complete_data($1); $$ = $2; }
;


Vardesc:
    NAME                          {
      std::string name = $1;
      $$ = std::make_pair(AST::Typ::pk_Void(), name);                                           }
  | STAR Vardesc                  { $$ = std::make_pair(AST::Typ::pk_Ptr($2.first), $2.second); }
  | LPAR Vardesc RPAR             { $$ = $2;                                                    }
  | Vardesc LBRACK RBRACK         {
      auto typ = AST::Typ::pk_Arr($1.first, std::nullopt);
      $$ = std::make_pair(typ, $1.second);                                                      }
  | Vardesc LBRACK CSTINT RBRACK  {
      auto typ = AST::Typ::pk_Arr($1.first, $3);
      $$ = std::make_pair(typ, $1.second);                                                      }
;




%%

AST::ExprHandle AccessAssign(std::string op, AST::AccessHandle dest, AST::ExprHandle expr) {
  auto mode = AST::Expr::Access::Mode::Access;
  auto acc = AST::Expr::pk_Access(mode, dest);
  auto r_expr = AST::Expr::pk_Prim2(op, acc, expr);
  return r_expr;
}

void yy::parser::error (const location_type& l, const std::string& m) {
  std::cerr << l << ": " << m << '\n';
}
