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

  struct Driver;
}

%param { Driver& drv } // Parsing context

%locations

%define parse.trace
%define parse.error detailed
%define parse.lac full

%code {
#include "driver.hh"

#include "AST/Node/Access.hh"
#include "AST/Node/Dec.hh"
#include "AST/Node/Expr.hh"
#include "AST/Node/Stmt.hh"

#include "AST/Types.hh"



AST::ExprHandle AccessAssign(Driver &drv, std::string op, AST::AccessHandle dest, AST::ExprHandle expr);
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

%nterm <AST::BlockHandle> Block
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
    NAME                       { $$ = drv.pk_AccessVar($1);              }
  | LPAR Access RPAR           { $$ = $2;                                }
  | STAR Access                {
      auto acc = drv.pk_ExprAccess(AST::Expr::Access::Mode::Access, $2);
      $$ = drv.pk_AccessDeref(std::move(acc));                           }
  | STAR AtExprNotAccess       { $$ = drv.pk_AccessDeref($2);            }
  | Access LBRACK Expr RBRACK  { $$ = drv.pk_AccessIndex($1, $3);        }
;


AtExprNotAccess:
    Const                    { $$ = $1;                                                   }
  | LPAR ExprNotAccess RPAR  { $$ = $2;                                                   }
  | AMP Access               { $$ = drv.pk_ExprAccess(AST::Expr::Access::Mode::Addr, $2); }
;


Block:
    LBRACE StmtOrDecSeq RBRACE
      { std::reverse($2.begin(), $2.end());
        $$ = drv.pk_StmtBlock(std::move($2));
      }
;


Const:
    CSTINT        { $$ = drv.pk_ExprCstI($1);                        }
  | CSTBOOL       { $$ = std::move(drv.pk_ExprCstI($1));             }
  | MINUS CSTINT  { $$ = drv.pk_ExprPrim1("-", drv.pk_ExprCstI($2)); }
  | NULL          { $$ = drv.pk_ExprPrim1("-", drv.pk_ExprCstI(1));  }
;


DataType:
    INT   { $$ = AST::Typ::Data::Int;  }
  | CHAR  { $$ = AST::Typ::Data::Char; }
;


Expr:
    Access         { $$ = drv.pk_ExprAccess(AST::Expr::Access::Mode::Access, $1); }
  | ExprNotAccess  { $$ = $1;                                                     }
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
    AtExprNotAccess           { $$ = $1;                                  }
  | Access ASSIGN Expr        { $$ = drv.pk_ExprAssign($1, $3);           }
  | Access PLUS_ASSIGN Expr   { $$ = AccessAssign(drv, "+", $1, $3);      }
  | Access MINUS_ASSIGN Expr  { $$ = AccessAssign(drv, "-", $1, $3);      }
  | Access STAR_ASSIGN Expr   { $$ = AccessAssign(drv, "*", $1, $3);      }
  | Access SLASH_ASSIGN Expr  { $$ = AccessAssign(drv, "/", $1, $3);      }
  | Access MOD_ASSIGN Expr    { $$ = AccessAssign(drv, "%", $1, $3);      }
  | NAME LPAR Exprs RPAR      { $$ = drv.pk_ExprCall($1, $3);             }
  | NOT Expr                  { $$ = drv.pk_ExprPrim1("!", $2);           }
  | PRINT Expr                { $$ = drv.pk_ExprPrim1("printi", $2);      }
  | PRINTLN                   { $$ = drv.pk_ExprPrim1("printc", drv.pk_ExprCstI(10)); }
  | Expr PLUS  Expr           { $$ = drv.pk_ExprPrim2("+",  $1, $3);      }
  | Expr MINUS Expr           { $$ = drv.pk_ExprPrim2("-",  $1, $3);      }
  | Expr STAR  Expr           { $$ = drv.pk_ExprPrim2("*",  $1, $3);      }
  | Expr SLASH Expr           { $$ = drv.pk_ExprPrim2("/",  $1, $3);      }
  | Expr MOD   Expr           { $$ = drv.pk_ExprPrim2("%",  $1, $3);      }
  | Expr EQ    Expr           { $$ = drv.pk_ExprPrim2("==", $1, $3);      }
  | Expr NE    Expr           { $$ = drv.pk_ExprPrim2("!=", $1, $3);      }
  | Expr GT    Expr           { $$ = drv.pk_ExprPrim2(">",  $1, $3);      }
  | Expr LT    Expr           { $$ = drv.pk_ExprPrim2("<",  $1, $3);      }
  | Expr GE    Expr           { $$ = drv.pk_ExprPrim2(">=", $1, $3);      }
  | Expr LE    Expr           { $$ = drv.pk_ExprPrim2("<=", $1, $3);      }
  | Expr SEQAND Expr          { $$ = drv.pk_ExprPrim2("&&", $1, $3);      }
  | Expr SEQOR  Expr          { $$ = drv.pk_ExprPrim2("||", $1, $3);      }
;


Fndec:
    VOID NAME LPAR Paramdecs RPAR Block      {
      auto r_typ = AST::Typ::pk_Void();
      auto f = drv.pk_DecFn(r_typ, $2, $4, $6);
      $$ = f;                                   }
  | DataType NAME LPAR Paramdecs RPAR Block  {
      auto r_typ = AST::Typ::pk_Data($1);
      auto f = drv.pk_DecFn(r_typ, $2, $4, $6);
      $$ = f;                                   }
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
    Expr SEMI                           { $$ = drv.pk_StmtExpr($1);                   }
  | RETURN SEMI                         { $$ = drv.pk_StmtReturn(std::nullopt);       }
  | RETURN Expr SEMI                    { $$ = drv.pk_StmtReturn($2);                 }
  | Block                               { $$ = std::static_pointer_cast<AST::StmtT>($1); }
  | IF LPAR Expr RPAR StmtA ELSE StmtA  { $$ = drv.pk_StmtIf($3, $5, $7);             }
  | WHILE LPAR Expr RPAR StmtA          { $$ = drv.pk_StmtWhile($3, $5);              }
;


StmtB:
    IF LPAR Expr RPAR StmtA ELSE StmtB  { $$ = drv.pk_StmtIf($3, $5, $7); }
  | IF LPAR Expr RPAR Stmt              {
      auto empty_block = drv.pk_StmtBlockStmt(AST::BlockVec{});
      $$ = drv.pk_StmtIf($3, $5, empty_block);                            }
  | WHILE LPAR Expr RPAR StmtB          { $$ = drv.pk_StmtWhile($3, $5);  }
;


StmtOrDecSeq:
    %empty                    { $$ = AST::BlockVec{};                           }
  | Stmt StmtOrDecSeq         { $2.push_back($1); $$ = $2;                      }
  | Vardec SEMI StmtOrDecSeq  {
      auto dec = drv.pk_DecVar(AST::Dec::Scope::Local, $1.first, $1.second);
      $3.push_back(dec);
      $$ = $3;                                                                  }
;





Topdecs:
    %empty          {  }
  | Topdec Topdecs  {  }
;


Topdec:
   Vardec SEMI  {
     auto dec = drv.pk_DecVar(AST::Dec::Scope::Global, $1.first, $1.second);
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

AST::ExprHandle AccessAssign(Driver &drv,  std::string op, AST::AccessHandle dest, AST::ExprHandle expr) {
  auto mode = AST::Expr::Access::Mode::Access;
  auto acc = drv.pk_ExprAccess(mode, dest);
  auto r_expr = drv.pk_ExprPrim2(op, acc, expr);
  return r_expr;
}

void yy::parser::error (const location_type& l, const std::string& m) {
  std::cerr << l << ": " << m << '\n';
}
