%{
#include <climits>
#include <string>
#include "Driver.hpp"
#include "parser.hpp"

std::string str_buf{};
%}

%{

%}

%option noyywrap nounput noinput batch debug

%x BLOCK_COMMENT
%x LINE_COMMENT
%x QUOTES


%{

%}

blank [ \r\t]
int   [0-9]+
name  [a-zA-Z][a-zA-Z_0-9]*



%{
  # define YY_USER_ACTION  loc.columns(yyleng);
%}


%%


%{
  yy::location& loc = drv.location;
  loc.step();
%}


{blank}+  loc.step();
\n+       loc.lines(yyleng); loc.step();
{int}     {
            int64_t n = strtoll(yytext, NULL, 10);
            if (!(INT64_MIN <= n && n <= INT64_MAX)) {
              throw yy::parser::syntax_error (loc, "Integer is out of range: " + std::string(yytext));
            }
            return yy::parser::make_CSTINT(n, loc);
          }

"char"    return yy::parser::make_CHAR    (loc);
"else"    return yy::parser::make_ELSE    (loc);
"false"   return yy::parser::make_CSTBOOL (0, loc);
"for"     return yy::parser::make_FOR     (loc);
"if"      return yy::parser::make_IF      (loc);
"int"     return yy::parser::make_INT     (loc);
"null"    return yy::parser::make_NULL    (loc);
"print"   return yy::parser::make_PRINT   (loc);
"println" return yy::parser::make_PRINTLN (loc);
"return"  return yy::parser::make_RETURN  (loc);
"true"    return yy::parser::make_CSTBOOL (1, loc);
"void"    return yy::parser::make_VOID    (loc);
"while"   return yy::parser::make_WHILE   (loc);

{name}    return yy::parser::make_NAME    (yytext, loc);

"+"       return yy::parser::make_PLUS         (loc);
"-"       return yy::parser::make_MINUS        (loc);
"*"       return yy::parser::make_STAR         (loc);
"/"       return yy::parser::make_SLASH        (loc);
"%"       return yy::parser::make_MOD          (loc);

"+="      return yy::parser::make_PLUS_ASSIGN  (loc);
"-="      return yy::parser::make_MINUS_ASSIGN (loc);
"%="      return yy::parser::make_MOD_ASSIGN   (loc);
"*="      return yy::parser::make_STAR_ASSIGN (loc);
"/="      return yy::parser::make_SLASH_ASSIGN   (loc);

"++"      return yy::parser::make_INC          (loc);
"--"      return yy::parser::make_DEC          (loc);

"="       return yy::parser::make_ASSIGN       (loc);

"=="      return yy::parser::make_EQ           (loc);
"!="      return yy::parser::make_NE           (loc);
">"       return yy::parser::make_GT           (loc);
"<"       return yy::parser::make_LT           (loc);
">="      return yy::parser::make_GE           (loc);
"<="      return yy::parser::make_LE           (loc);

"||"      return yy::parser::make_SEQOR        (loc);
"&&"      return yy::parser::make_SEQAND       (loc);

"&"       return yy::parser::make_AMP          (loc);
"!"       return yy::parser::make_NOT          (loc);

"("       return yy::parser::make_LPAR         (loc);
")"       return yy::parser::make_RPAR         (loc);
"{"       return yy::parser::make_LBRACE       (loc);
"}"       return yy::parser::make_RBRACE       (loc);
"["       return yy::parser::make_LBRACK       (loc);
"]"       return yy::parser::make_RBRACK       (loc);

";"       return yy::parser::make_SEMI         (loc);

":"       return yy::parser::make_COLON        (loc);
"?"       return yy::parser::make_QMARK        (loc);

","       return yy::parser::make_COMMA        (loc);

<INITIAL>{
  "/*" BEGIN(BLOCK_COMMENT);
  "//" BEGIN(LINE_COMMENT);
  "\"" BEGIN(QUOTES);
}

<BLOCK_COMMENT>{
  "*/" BEGIN(INITIAL);
  "*"     //
  [^\n]   //
  \n      yylineno++;
  <<EOF>> { throw yy::parser::syntax_error(loc, "Unterminated comment"); }
}

<LINE_COMMENT>{
  [^\n]+  //
  \n      yylineno++; BEGIN(INITIAL);
}

<QUOTES>{
  "\""    {
            BEGIN(INITIAL);
            std::string str{};
            std::swap(str_buf, str);
            return yy::parser::make_CSTSTRING (str, loc);
          }
  "\'"    { str_buf.push_back('\\'); str_buf.push_back('\''); }
  \n      { throw yy::parser::syntax_error(loc, "Newline in string: " + std::string(yytext)); }
  [^\"]   str_buf.push_back(*yytext);
  <<EOF>> { throw yy::parser::syntax_error(loc, "Unterminated string"); }
}

. { throw yy::parser::syntax_error(loc, "Invalid character: " + std::string(yytext)); }

<<EOF>>    return yy::parser::make_YYEOF (loc);


%%


void Driver::scan_begin () {
  yy_flex_debug = trace_scanning;
  if (file.empty () || file == "-") {
    yyin = stdin;
  }
  else if (!(yyin = fopen (file.c_str (), "r"))) {
    std::cerr << "Unable to open " << file << ": " << strerror (errno) << '\n';
    exit(EXIT_FAILURE);
  }
}

void Driver::scan_end () {
  fclose(yyin);
}
