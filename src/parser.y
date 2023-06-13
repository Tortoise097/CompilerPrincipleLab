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
#include <cmath>

//声明 lexer 函数和错误处理函数
int     yylex();
void    yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

  // global variables in ast.cpp
  // actually they should be implemented as inherited attributes
  // but currently i don't know how to use inheritted attributes in bison

/*
extern SymtabList glb_symtab_list; 
extern int label_count;
extern vector<int> while_stack;
extern vector<int> end_stack;
extern bool main_certain_return;
*/

%}

  /* defines the YYSTYPE as below: */
  /* The possible return value taken from yylval*/
%union {
  std::string* str_ptr;
  int int_val;
  BaseAST* ast_ptr;
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


%token <str_ptr> IDENT
%token <int_val> INT_CONST


  /* %type for nonterminals */
  /* Take care of the precedence, 
   * These nonterminals should be written from "top" to "bottom"
   */
%type <int_val> Exp UnaryExp AddExp LOrExp
%type <int_val> RelExp EqExp LAndExp  MulExp
%type <int_val> Number PrimaryExp LVal
    /* temporarily ignore Btype since it can only be i32
    %type <str_ptr> Btype
    */


%type <ast_ptr> FuncDef
%type <ast_ptr> FuncType
%type <ast_ptr> Block RepBlockItem BlockItem
%type <ast_ptr> Decl Stmt non_if_else_stmt if_else_stmt open_stmt closed_stmt
%type <ast_ptr> VarDecl ReVarDef VarDef
  /* Since now we skip error handling, those who 
  do not generate IR just don't need to return anything*/

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
    //make_unique: create a new unique_ptr with a new object
    auto comp_unit = make_unique<CompUnitAST>();

    //initialize global
    label_count = 0;
    main_certain_return = 0;

      /* Initialize the glb_symtab_list in the root notde*/
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit); //generate AST
    }
  ;

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST(); // 在 Bison 生成的 parser 中完成了 AST 的构建, 并将生成的 AST 返回给了 parser 函数的调用者.
      /* Here S1 is a *ast, unique_ptr<BaseAST>(S1) *Turns* S1 into a unique_ptr
        But be aware that if S1 is a unique_ptr, you have to use move.
      */
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
    }
  ;

FuncType
  : INT {
    auto ast = new FuncTypeAST(": i32");
    $$ = ast;
    }
  | VOID {
    auto ast = new FuncTypeAST(" ");
    $$ = ast;
    }
  ;

  /* Block::= "{" {BlockItem} "}"; */
Block
  : '{' '}' {
    auto blc_ast = new BlockAST();
    blc_ast->repBlockItems = make_unique<EmptyAST>();
    $$ = blc_ast;
    }
  | '{' {  /* See { push */
    // make a new symbol table
    glb_symtab_list.symList.push_back(make_unique<SymTab>());

    }
    RepBlockItem '}' { /*See } pop*/
    // Generate IR
    auto blc_ast = new BlockAST();
    blc_ast->repBlockItems = unique_ptr<BaseAST>($3);
    glb_symtab_list.symList.pop_back();
    // cout<<"line 137, returned in RepBlockItem in Block"<<endl;
    $$ = blc_ast ;
    }
  ;

RepBlockItem
  : BlockItem {
    auto rep_ast = new RepBlockItemAST ();
      /*create a new empty vector */
    rep_ast->rep_vec_ptr = make_unique<vector<std::unique_ptr<BaseAST>>>(); 
    (*(rep_ast->rep_vec_ptr)).push_back(unique_ptr<BaseAST>($1));  // add and initialize an unique_ptr
    $$ = rep_ast;
    }
  | RepBlockItem BlockItem {
      /* S1 has the pointer that passed by the previous case's BlockItem */
    // cout<<"in line 153, BlockItem returned"<<endl;
    (*(($1)->rep_vec_ptr)).push_back(unique_ptr<BaseAST>($2));
    // cout<<"line 154"<<endl;
    $$ = $1;
    }
  ;

  /* BlockItem return ast to Block */
BlockItem   
  : Decl { /*return AST, dump */ }  
  | Stmt { /*return AST, dump*/  } 
  ;

Decl
  : ConstDecl { 
    /* we just write an empty class that donot dump anything, 
    also it may print some error and pass it to the former*/
    auto ast = new EmptyAST();
    $$ = ast;
    }
  | VarDecl 
  ;

  /* 
    The ConstDecl do not generate IR code, only insert a const value in the symtab
    The "insert in symbtab" action was done in ConstDef
    Others just do nothing
  */
ConstDecl
  : CONST Btype ReConstDef ';'  
  ;

  /* Repeated unit in ConstDef */
  /* Special Note: 
    actually here we should use the "btype" when we want to 
    add a new const to the symbtab.
    but this will need to use the inherited attributed btype.
    It seems that yacc/bison do not suppor inherited attributes,
    maybe I can use a global variable, but i think it's risky.
    Since in this project the Btype can only be "i32"
    Now I just skip it and make all Btype as "i32"
    (although it shouldn't be like this orz)
  */
ReConstDef   
  : ConstDef
  | ReConstDef ',' ConstDef  
  ;

ConstDef
  : IDENT '=' Exp { 
    /*insert the IDENT in symbtab, no return unless debug*/
    // No IR generated, only add to symtab, 
    // use the original name.
    glb_symtab_list.symList.back()->add_new_var(*unique_ptr<string>($1), $3);
    }
  ;

VarDecl
  : Btype ReVarDef ';' { $$ = $2; }
  ;

ReVarDef
  : VarDef {
    auto rep_ast = new RepVarDeclAST ();
      /*prepare for IR generation*/
    //cout<<"line 218 ok"<<endl;
    rep_ast->rep_vec_ptr = make_unique<vector<std::unique_ptr<BaseAST>>>(); 
    (*(rep_ast->rep_vec_ptr)).push_back(unique_ptr<BaseAST>($1)); 
    // cout<<"line 220 ok"<<endl;
    $$ = rep_ast;
  }
  | ReVarDef ',' VarDef { 
    (*(($1)->rep_vec_ptr)).push_back(unique_ptr<BaseAST>($3));
    $$ = $1;
  }
  ;


VarDef
  : IDENT {
    /*insert the IDENT in symbtab, build ast and generate IR code */ 
    /* Uninitialized variables be initialized to 0*/
    // Generate IR: using the unique_name
    // add_to_symtablist: use the original name
    string name = *($1);
    // cout<<"In parser.y line 234, name = "<<name <<endl;
    string unique_name = name + "_" + to_string(glb_symtab_list.symList.size());
    // cout<<"In parser.y line 236, unique_name = "<<unique_name <<endl;
    auto ast = new VarDefAST(unique_name, 0, 0);
    // cout<<"In parser.y line 238"<<endl;
    glb_symtab_list.symList.back()->add_new_var(name, 0);
    // cout<<"In parser.y line 239"<<endl;
    auto p = unique_ptr<string>($1); //auto delete string s1
    $$ = ast;
    } /* Var can be initialized or unintialized*/
  | IDENT '=' Exp { 
    string name = *($1);
    string unique_name = name + "_" + to_string(glb_symtab_list.symList.size());
    auto ast = new VarDefAST(unique_name, $3, 1); //Used to generate IR
    // Add to symtab
    glb_symtab_list.symList.back()->add_new_var(name, $3);
    auto p = unique_ptr<string>($1); //auto delete string s1
    $$ = ast; 
    }
  ;


  /* Temporarily ignore Btype since it can only be i32*/
Btype
  : INT  
  ;

Stmt
  : non_if_else_stmt
  | if_else_stmt 
  ;

if_else_stmt
  :open_stmt {cout<<"open stmt"<<endl;}
  |closed_stmt  {cout<<"closed_stmt"<<endl;}
  ;

open_stmt
  : IF '(' Exp ')' {
      end_stack.push_back(label_count);
      if_stack.push_back(label_count);
      label_count +=1;
      cout<<"open_stmt, Line 293, label_count = "<<label_count<<endl;
      PrintStack();
    }
    if_else_stmt{
      string s1,s2, s3;
      int k = if_stack.back();
      s1 = "%then_" + to_string(k);
      s2 = "%else_" + to_string(k);
      s3 = "%end_" +to_string(k);
      auto ast = new IfThenElseAST($3,s1,s2,s3);
      ast->br_then = unique_ptr<BaseAST>($6);
        /* It is an empty Block but since block in Koopa cannot be empty*/
        /* insert a jump */
      ast->br_else = make_unique<EmptyAST>();
      ast->jmp_to_next_end = EndJumping();
      cout<<"end_jump = "<<EndJumping()<<endl;
      cout<<"line 309, pop"<<endl;
      end_stack.pop_back();
      if_stack.pop_back();
      cout<<"Line 310, label_count = "<<label_count<<endl;
      cout<<"After pop, printstack"<<endl;
      PrintStack();
      $$ = ast;
    }
  | IF '(' Exp ')' {
      end_stack.push_back(label_count);
      if_stack.push_back(label_count);
      label_count +=1;
      cout<<"open_stmt Line 320, label_count = "<<label_count<<endl;
      PrintStack();
    }
      closed_stmt ELSE open_stmt{
      string s1,s2,s3;
      int k = if_stack.back();
      s1 = "%then_" + to_string(k);
      s2 = "%else_" + to_string(k);
      s3 = "%end_" + to_string(k);
      auto ast = new IfThenElseAST($3,s1,s2,s3);
      ast->br_then = unique_ptr<BaseAST>($6);
      ast->br_else = unique_ptr<BaseAST>($8);
      ast->jmp_to_next_end = EndJumping();
      cout<<"end_jump = "<<EndJumping()<<endl;
      cout<<"line 334, pop"<<endl;
      end_stack.pop_back();
      if_stack.pop_back();
      cout<<"Line 310, label_count = "<<label_count<<endl;
      cout<<"After pop, printstack"<<endl;
      PrintStack();
      $$ = ast;
    }
  ;

closed_stmt
  : non_if_else_stmt
  | IF '(' Exp ')'{
      end_stack.push_back(label_count);
      if_stack.push_back(label_count);
      label_count +=1;
      cout<<"Line 348, label_count = "<<label_count<<endl;
      PrintStack();
    } 
    closed_stmt ELSE closed_stmt{
      string s1,s2,s3;
      int k = if_stack.back();
      s1 = "%then_" + to_string(k);
      s2 = "%else_" + to_string(k);
      s3 = "%end_" +to_string(k);
      auto ast = new IfThenElseAST($3,s1,s2,s3);
      ast->br_then = unique_ptr<BaseAST>($6);
      ast->br_else = unique_ptr<BaseAST>($8);
      ast->jmp_to_next_end = EndJumping();
      cout<<"end_jump = "<<EndJumping()<<endl;
      cout<<"line 363, pop"<<endl;
      end_stack.pop_back();
      if_stack.pop_back();
      cout<<"Line 365, label_count = "<<label_count<<endl;
      cout<<"After pop, printstack"<<endl;
      PrintStack();
      $$ = ast;
    }
  ;


non_if_else_stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST(StmtType::_returnVar);
    ast->val = $2;
    cout<<"In parser.y line 263 ret va = "<< ast->val <<endl;
    $$ = ast;
    }
  | RETURN ';' { 
    auto ast = new StmtAST(StmtType::_return);
    $$ = ast;
    } 
  | Block { 
    auto ast = new StmtAST(StmtType::_block);
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
    } 
  | ';'  { 
    auto ast = new EmptyAST();
    $$ = ast;
    } 
  | Exp ';'  {
     /* An empty calc, return EmptyAST that donot dump anything. 
     This can be optimized! */ 
     auto ast = new EmptyAST();
     $$ = ast;
     } 
  | IDENT '=' Exp ';' { 
     /*assignment statement*/
    // use S1 to find the variable stored in the symbtable
    // and change its value to S3 
    // generate IR
    // cout<<"line 293, assignment"<<endl;
    auto ast = new StmtAST(StmtType::_assign);
    ast->val = $3;
    string name = *($1);
    // It has to know which variable it is assgining to
    int postfix = glb_symtab_list.ChangeValue(name, $3); // everything is IR is the unique_name
    // but everything in symtab is the original name
    string unique_name = name + "_" + to_string(postfix);
    ast->name = unique_name; //assign also use the unique_name
    // change symtab:
    // cout<<"line 298, ChangeValue"<<endl;
    auto p = unique_ptr<string>($1); //auto delete string s1
    $$ = ast;
    }
  | WHILE '(' Exp ')' {
    while_stack.push_back(label_count);
    end_stack.push_back(label_count);
    label_count += 1;
    }
    Stmt { 
      string s1,s2;
      int in_which_while = while_stack.back();
      s1 = "%while_entry_" + to_string(in_which_while);
      s2 = "%while_end_" + to_string(in_which_while);
      auto ast = new WhileAST($3,s1,s2);
      ast->body = unique_ptr<BaseAST>($6);  
      ast->jmp_to_next_end = EndJumping();
      while_stack.pop_back();
      end_stack.pop_back();
      $$ = ast;
    }
  | BREAK ';' {
      int in_which_while = while_stack.back();
      auto ast = new StmtAST(StmtType::_break);
      ast->name = "%while_end_" +to_string(in_which_while);
      $$ = ast;
    }
  | CONTINUE ';'{
      int in_which_while = while_stack.back();
      auto ast = new StmtAST(StmtType::_continue);
      ast->name = "%while_entry_" +to_string(in_which_while);
      $$ = ast;
    }
  ;

  /* Everything appears in the exp should be an int, so we can calculate them directly*/
  /* Revising the Exp parts to meet the request of Lv5 */
  /* //%type<int_val> LVal, Lval: IDENT{$$ = findValInSymtab($1);} */
Exp
  : LOrExp 
  ;


UnaryExp
  :  PrimaryExp  
  | '+' UnaryExp {$$ = $2; }
  | '-' UnaryExp {$$ = 0 - $2;}
  | '!' UnaryExp {$$ = !$2; }
  ;

AddExp
  : MulExp
  | AddExp '+' MulExp {$$ = $1 + $3;}
  | AddExp '-' MulExp {$$ = $1 - $3;}
  ;
  
MulExp
  : UnaryExp
  | MulExp '*' UnaryExp {$$ = $1 * $3;}
  | MulExp '/' UnaryExp {$$ = $1 / $3;}
  | MulExp '%' UnaryExp {$$ = fmod($1, $3);}
  ;

RelExp
  : AddExp
  | RelExp '<' AddExp {$$ = $1 < $3; }
  | RelExp '>' AddExp {$$ = $1 > $3; }
  | RelExp LE AddExp {$$ = $1 <= $3; }
  | RelExp GE AddExp {$$ = $1 >= $3; }
  ;

EqExp
  : RelExp
  | EqExp EQ RelExp {$$ = $1 == $3; }
  | EqExp NE RelExp {$$ = $1 != $3; }
  ;

LAndExp
  : EqExp
  | LAndExp AND  EqExp {$$ = $1 && $3;}
  ;

LOrExp
  : LAndExp
  | LOrExp OR LAndExp {$$ = $1 || $3;}
  ;

PrimaryExp
  : '(' Exp ')' {$$ = $2;}
  | LVal
  | Number 
  ; 

Number
  : INT_CONST
;

LVal
  : IDENT{
    string name = *($1);
    string s = glb_symtab_list.GetValue(name);
      if(s == "NotFound"){
        printf("Something wrong in line 366, not found.\n");
      }
      else{
        auto p = unique_ptr<string>($1); //auto delete string s1
        $$ = stoi(s);
      }
    } 
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
