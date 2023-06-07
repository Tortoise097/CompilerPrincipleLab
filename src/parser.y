%code requires {
  #include <memory>
  #include <string>
  #include "ast.hpp"
}

%{
#include <iostream>
#include <memory>
#include <string>
#include "ast.hpp"

//声明 lexer 函数和错误处理函数
int     yylex();
void    yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

  /* defines the YYSTYPE as below: */
  /* The possible return value taken from yylval*/
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  FuncType func_type;
  VarType var_type;
  DeclType decl_type;
  StmtType stmt_type;
}

  /* Declare additional arguments for yyparse */
%parse-param { std::unique_ptr<BaseAST> &ast }


// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
  /* Note that the precedence is specified in increasing precedence from top to bottom, 
   * and tokens on the same line are given the same precedence.
   */

%token INT CONST VOID
%token IF ELSE WHILE BREAK CONTINUE RETURN
%token AND OR EQ NE LE GE
%token PLUS MINUS
%token MUL DIV
%token NOT 

%token <str_val> IDENT
%token <int_val> INT_CONST


  /* %type for nonterminals */
  /* Take care of the precedence, 
   * These nonterminals should be written from "top" to "bottom"
   */
%type <ast_val> FuncDef Block Stmt
%type <str_val> Number Exp
%type <func_type> FuncType

%%
  /*  Grammar here:
      CompUnit ::= FuncDef ;
      FuncDef ::= FuncType IDENT '(' ')' Block; 
      CompUnit  ::= FuncDef; 
 
      FuncDef   ::= FuncType IDENT "(" ")" Block; 
      FuncType  ::= "int"; 
      
      Block     ::= "{" Stmt "}"; 
      Stmt      ::= "return" Number ";"; 
      Number    ::= INT_CONST;
   */

CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit); //generate AST
  }
  ;

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST(); // 在 Bison 生成的 parser 中完成了 AST 的构建, 并将生成的 AST 返回给了 parser 函数的调用者.
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
;

FuncType
  : INT {
    auto ast = new FuncTypeAST(FuncType::_int);
    $$ = ast;
  }
  ;

Block
  : '{' Stmt '}' {
    auto ast = new BlockAST();
    ast->stmt = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
;

Stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST(StmtType::_return);
    ast->expr = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
;

Exp
  : UnaryExp{

  }
;

PrimaryExp
  : '(' Exp ')' 
  |  Number
  ; 

UnaryExp
  :  PrimaryExp 
  | '+' UnaryExp
  | '-' UnaryExp
  | '!' UnaryExp
  ;


  /* UnaryOp 
  : '+' 
  | '-' 
  | '!'
  ; */

Number
  : INT_CONST {
    $$ = new string(to_string($1));
  }
;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
