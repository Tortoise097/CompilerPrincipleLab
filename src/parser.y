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
%type <ast_ptr> Exp UnaryExp AddExp LOrExp
%type <ast_ptr> RelExp EqExp LAndExp  MulExp PrimaryExp
%type <int_val> Number

    /* temporarily ignore Btype since it can only be i32
    %type <str_ptr> Btype
    */


%type <ast_ptr> FuncDef
%type <ast_ptr> FuncType
%type <ast_ptr> Block RepBlockItem BlockItem
%type <ast_ptr> Decl Stmt
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
    block_ended = 0;
    IR_temp_var_count = 0;
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
      if(!($3)->expr_pure_value){
        //this Exp must be a constExp which means this wouldn't happen
        cout<<"error, cannot initialize a const"<<endl;
      }
      else{
        int val = stoi(($3)->expr_result);
        //set unique_name to default, because unique_name is only used in generating IR
        // but const do not generate any IR code
        glb_symtab_list.symList.back()->add_new_var(*($1), val, *($1), 1, VarType::_const );
        auto p =  unique_ptr<string>($1);
        auto p2 = unique_ptr<BaseAST>($3); //delete S3
      }
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
    // Generally I think tihs is a smart way to generate unique_name, 
    // but koopa don't allow,
    // so I have to use another way
    // string unique_name = name + "_" + to_string(glb_symtab_list.symList.size());
    string unique_name = "@" + glb_symtab_list.GenUniqueName(name);
    // cout<<"In parser.y line 236, unique_name = "<<unique_name <<endl;
    
    auto ast = new VarDefAST(unique_name, 0);
    // Noted that every variable is by default set to 0
    glb_symtab_list.symList.back()->add_new_var(name, 0, unique_name,0, VarType::_var);
    // cout<<"In parser.y line 239"<<endl;
    auto p = unique_ptr<string>($1); //auto delete string s1
    $$ = ast;
    } /* Var can be initialized or unintialized*/
  | IDENT '=' Exp { 
    string name = *($1);
    // cout<<"274"<<endl;
    string unique_name = "@" + glb_symtab_list.GenUniqueName(name);
    auto ast = new VarDefAST(unique_name, 1); //Used to generate IR
    ast->expr = unique_ptr<BaseAST>($3);
    // Add to symtab
    // Noted that the "value" is set to 0 ,but we will not use the variable's value from symbtab
    glb_symtab_list.symList.back()->add_new_var(name, 0, unique_name, 1, VarType::_var);
    auto p = unique_ptr<string>($1); //auto delete string s1
    $$ = ast; 
    }
  ;


  /* Temporarily ignore Btype since it can only be i32*/
Btype
  : INT  
  ;

Stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST(StmtType::_returnVar);
    ast->expr = unique_ptr<BaseAST>($2);
    //cout<<"In parser.y line 263 ret va = "<< ast->val <<endl;
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
     /* do not generate any IR code*/ 
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
    ast->expr = unique_ptr<BaseAST>($3);
    string name = *($1);
    // It has to know which variable it is assgining to
    //int postfix = glb_symtab_list.ChangeValue(name, $3);
    // glb_symtab_list.ChangeValue(name, $3); // everything is IR is the unique_name
    // but everything in symtab is the original name
    string unique_name = glb_symtab_list.GetUniqueName(name);
    // string unique_name = name + "_" + to_string(postfix);
    ast->name = unique_name; //assign also use the unique_name
    // change symtab:
    // cout<<"line 298, ChangeValue"<<endl;
    auto p = unique_ptr<string>($1); //auto delete string s1
    $$ = ast;
    }
  | IF '(' Exp ')' Stmt{
      auto ast = new IfThenElseAST();
      ast->expr = unique_ptr<BaseAST>($3);
      ast->br_then = unique_ptr<BaseAST>($5);
      ast->br_else = make_unique<EmptyAST>();
      $$ = ast;
    }
  | IF '(' Exp ')' Stmt ELSE Stmt{
      auto ast = new IfThenElseAST();
      ast->expr = unique_ptr<BaseAST>($3);
      ast->br_then = unique_ptr<BaseAST>($5);
      ast->br_else = unique_ptr<BaseAST>($7);
      $$ = ast;
    }
  | WHILE '(' Exp ')' Stmt { 
      auto ast = new WhileAST();
      ast->expr = unique_ptr<BaseAST>($3);
      ast->body = unique_ptr<BaseAST>($5);  
      $$ = ast;
    }
  | BREAK ';' {
      auto ast = new StmtAST(StmtType::_break);
      //ast->name = "%while_end_" +to_string(in_which_while);
      $$ = ast;
    }
  | CONTINUE ';'{
      auto ast = new StmtAST(StmtType::_continue);
      // ast->name = "%while_entry_" +to_string(in_which_while);
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
  | '-' UnaryExp {
      auto ast = new ExprAST();
      if(($2)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(0 - stoi(($2)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($2);
        ast->opt = ExpType::_minus;  
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1;
      }
      $$ = ast;
    }
  | '!' UnaryExp {
      auto ast = new ExprAST();
      if(($2)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(!stoi(($2)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($2);
        ast->opt = ExpType::_not;  
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1;
      }
      $$ = ast;
    }
  ;

AddExp
  : MulExp
  | AddExp '+' MulExp {
      auto ast = new ExprAST();
      if(($1)->expr_pure_value && ($3)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(stoi(($1)->expr_result) + stoi(($3)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($1);
        ast->operand2 = unique_ptr<BaseAST>($3);
        ast->opt = ExpType::_add;  
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1;
      }
      $$ = ast;
    }
  | AddExp '-' MulExp {
      auto ast = new ExprAST();
      if(($1)->expr_pure_value && ($3)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(stoi(($1)->expr_result) - stoi(($3)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($1);
        ast->operand2 = unique_ptr<BaseAST>($3);
        ast->opt = ExpType::_sub;  
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1;
      }
      $$ = ast;
    }
  ;
  
MulExp
  : UnaryExp
  | MulExp '*' UnaryExp {
      auto ast = new ExprAST();
      if(($1)->expr_pure_value && ($3)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(stoi(($1)->expr_result) * stoi(($3)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($1);
        ast->operand2 = unique_ptr<BaseAST>($3);
        ast->opt = ExpType::_mul;  
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1;
      }
      $$ = ast;
    }
  | MulExp '/' UnaryExp {
      auto ast = new ExprAST();
      if(($1)->expr_pure_value && ($3)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(stoi(($1)->expr_result) / stoi(($3)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($1);
        ast->operand2 = unique_ptr<BaseAST>($3);
        ast->opt = ExpType::_div;  
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1;
      }
      $$ = ast;
    }
  | MulExp '%' UnaryExp {
      auto ast = new ExprAST();
      if(($1)->expr_pure_value && ($3)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(stoi(($1)->expr_result) % stoi(($3)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($1);
        ast->operand2 = unique_ptr<BaseAST>($3);
        ast->opt = ExpType::_mod;  
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1;
      }
      $$ = ast;}
  ;

RelExp
  : AddExp
  | RelExp '<' AddExp {
      auto ast = new ExprAST();
      if(($1)->expr_pure_value && ($3)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(stoi(($1)->expr_result) < stoi(($3)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($1);
        ast->operand2 = unique_ptr<BaseAST>($3);
        ast->opt = ExpType::_lt;  
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1;
      }
      $$ = ast;
    }
  | RelExp '>' AddExp {
      auto ast = new ExprAST();
      if(($1)->expr_pure_value && ($3)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(stoi(($1)->expr_result) > stoi(($3)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($1);
        ast->operand2 = unique_ptr<BaseAST>($3);
        ast->opt = ExpType::_gt;  
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1;
      }
      $$ = ast;
    }
  | RelExp LE AddExp {
      auto ast = new ExprAST();
      if(($1)->expr_pure_value && ($3)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(stoi(($1)->expr_result) <= stoi(($3)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($1);
        ast->operand2 = unique_ptr<BaseAST>($3);
        ast->opt = ExpType::_le;  
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1;
      }
      $$ = ast;
    }
  | RelExp GE AddExp {
      auto ast = new ExprAST();
      if(($1)->expr_pure_value && ($3)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(stoi(($1)->expr_result) >= stoi(($3)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($1);
        ast->operand2 = unique_ptr<BaseAST>($3);
        ast->opt = ExpType::_ge;  
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1;
      }
      $$ = ast;
    }
  ;

EqExp
  : RelExp
  | EqExp EQ RelExp {
    auto ast = new ExprAST();
      if(($1)->expr_pure_value && ($3)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(stoi(($1)->expr_result) == stoi(($3)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($1);
        ast->operand2 = unique_ptr<BaseAST>($3);
        ast->opt = ExpType::_eq;  
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1;
      }
      $$ = ast;
    }
  | EqExp NE RelExp {
      auto ast = new ExprAST();
      if(($1)->expr_pure_value && ($3)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(stoi(($1)->expr_result) != stoi(($3)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($1);
        ast->operand2 = unique_ptr<BaseAST>($3);
        ast->opt = ExpType::_ne; 
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1; 
      }
      $$ = ast;
    }
  ;

LAndExp
  : EqExp
  | LAndExp AND  EqExp {
      auto ast = new ExprAST();
      if(($1)->expr_pure_value && ($3)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(stoi(($1)->expr_result) && stoi(($3)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($1);
        ast->operand2 = unique_ptr<BaseAST>($3);
        ast->opt = ExpType::_and;  
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1;
      }
      $$ = ast;
    }
  ;

LOrExp
  : LAndExp
  | LOrExp OR LAndExp {
      auto ast = new ExprAST();
      if(($1)->expr_pure_value && ($3)->expr_pure_value){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(stoi(($1)->expr_result) || stoi(($3)->expr_result));
      }
      else{
        ast->expr_pure_value = 0;
        ast->operand1 = unique_ptr<BaseAST>($1);
        ast->operand2 = unique_ptr<BaseAST>($3);
        ast->opt = ExpType::_or;  
        ast->expr_result = "%" + to_string(IR_temp_var_count);
        IR_temp_var_count += 1;
      }
      $$ = ast;
    }
  ;

PrimaryExp
  : '(' Exp ')' {$$ = $2;}
  | IDENT{
      auto ast = new ExprAST();
      string name = *($1);
      auto symptr = unique_ptr<Symbol>(glb_symtab_list.GetInfo(name));
      if(symptr->sym_type == VarType::_const){
        ast->expr_pure_value = 1;
        ast->expr_result = to_string(symptr->val);
        // ast->former_exp = make_unique<EmptyAST>();
        ast->opt = ExpType::_Number;
      }
      else{
        // cout<<"In 657"<<endl;
        ast->expr_pure_value = 0; //already involves variable
        ast->expr_result = symptr->unique_name;
        // ast->former_exp = make_unique<EmptyAST>();
        ast->opt = ExpType::_Var;
      }
      $$ = ast;
    }
  | Number {
    // cout<<"In 665"<<endl;
    auto ast = new ExprAST();
    ast->expr_pure_value = 1;
    ast->expr_result = to_string($1);
    // ast->former_exp = make_unique<EmptyAST>();
    ast->opt = ExpType::_Number;
    $$ = ast;
    }
  ; 

Number
  : INT_CONST
;


%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
