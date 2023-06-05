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

%token <str_val> IDENT
%token <int_val> INT_CONST


  /* %type for nonterminals */
  /* Take care of the precedence, 
   * These nonterminals should be written from "top" to "bottom"
   */
%type <ast_val> FuncDef FuncType Block Stmt
%type <int_val> Number


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

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST(); // 在 Bison 生成的 parser 中完成了 AST 的构建, 并将生成的 AST 返回给了 parser 函数的调用者.
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
;
// 同上, 不再解释
FuncType
  : INT {
    $$ = new FuncTypeAST("i32");
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
  : RETURN Number ';' {
    auto ast = new StmtAST($2);
    $$ = ast;
  }
;

Number
  : INT_CONST {
    $$ = $1;
  }
;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
