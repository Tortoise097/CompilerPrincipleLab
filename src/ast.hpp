#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <stack>
#include <list>
#include <memory>
using namespace std;

enum class StmtType{
    _return,
    _returnVar,
    _block,
    _if,
    _else,
    _while,
    _assign, //a variable assignment
    _break,
    _continue,
};

enum class VarType{
    _const,
    _var,
};

enum class ExpType{
    _plus, //do nothing, or %k = add x, 0
    _minus, // %k = sub 0, x
    _not,  // %k = eq 0, x
    _add,  // %k = add x, y
    _sub,  // %k = sub x, y
    _mul,  // %k = mul x, y
    _div,  // %k = div x, y
    _mod,  // %k = mod x, y
    _lt, // <    // %k = lt x, y
    _gt, // >    // %k = gt x, y
    _le, // <=    // %k = le x, y
    _ge, // >=    // %k = ge x, y
    _eq,    // %k = eq x, y
    _ne,    // %k = ne x, y
    _and,    // %k = add x, y
    _or,    // %k = or x, y
    _Var,   // An variable   
    _Number, // a const 
};


//Gloal variables
extern int label_count;
extern vector<int> if_stack;
extern vector<int> while_stack;
extern vector<int> end_stack;
extern bool main_certain_return; //this should be func_certain_return,as an inherited attributed of func
extern bool block_ended;
extern int IR_temp_var_count; 

extern int temp_var_used_for_load;// actually it's a bug about IR_temp_var_count is set to 0 zfter parsing, so I have to use another temp_var_count, and initialized it to 1000

string EndJumping();
void PrintStack();

class Symbol{ //symbol management
public:
    int val;
    string unique_name;
    bool if_initialized;
    VarType sym_type;
    Symbol(int v, string s, bool b, VarType t):
        val(v),unique_name(s),if_initialized(b),
        sym_type(t){}
    Symbol(){}
    
};

class SymTab{
public:
    // 1.it do not store the Btype because btype can only be i32
    // 2.also it do not store the "alias", 'cause "alias" is calculated when used
    // 3.also we should store the variable's type (eg const or var)
    // but "trying to change a const's value" is a matter of error handling 
    // so I will not implement these feater here :p

    unordered_map<string, Symbol> mp; 

    //add_new_var(Name, value, unique_name, if_initialized, var_type)
    void add_new_var(string Name, int v, string s, bool b,VarType t){
        // actually here should have a error case, eg if declared a 
        // variable with the same name 

        mp.insert(make_pair(Name, Symbol(v,s,b,t)));
    }

    //unordered_map<string, int>::const_iterator
    string GetVar(string Name){
        auto it = mp.find(Name);
        if(it == mp.end()){
            return "NotFound";
        }
        else{
            return string(to_string((it->second).val));
        }
    }

    // Get whole information about a variable
    Symbol* GetInfo(string Name){
        auto it = mp.find(Name);
        if(it == mp.end()){
            return nullptr;
        }
        else{
            return new Symbol(it->second);
        }
    }

    string GetUniqueName(string Name){
        auto it = mp.find(Name);
        if(it == mp.end()){
            return ""; 
        }
        else{
            return string((it->second).unique_name);
        }
    }
    int ChangeValue(string Name, int value){
        auto it = mp.find(Name);
        if(it == mp.end()){
            return -1; // name not found
        }
        else{
            (it->second).val = value;
            return 1;
        }
    }
};

class SymtabList {
    public:
        // 1. a list that be managed in the ways of a stack, but can also searched forward
        vector<std::unique_ptr<SymTab>> symList; // initialize an empty list
        //keep record of variables of the same name, used to generate unique_name
        map<string, int> NameManagement;
        //use: auto temp_st = make_unique<SymTab>();
        // and after finished adding variables in SymTab, push_back this symtab
        // using stList.Push_back(std::move(temp_st));
        /* This function is correctly worked
        void Push_back(std::unique_ptr<SymTab> st){
            symList.push_back(std::move(st));
        }
        void Pop(){
            symList.pop_back();
        }
        */

        // GetTop()// get the top of the list, return a unique_ptr. use move



        // GetValue: use the identifier to get the value of the least-recently declared variable of that Name
        // if not found, return "NotFound"
        // Noted that the GetValue returns int value as a string.
        // it should be turned back to int before use.

        string GenUniqueName(string s){//Generate Unique_name for a new declared variable
            if(NameManagement.find(s) == NameManagement.end()){//the first copy
                NameManagement[s] = 1;
            }
            else{
                NameManagement[s] += 1;
            }
            return s + '_' + to_string(NameManagement[s]);
        }
        string GetValue(string Name){
            if(symList.empty()){
                return "NotFound";
            }

            for(auto it = symList.end(); it!=symList.begin(); ){
                --it;
                string rr = (*it)->GetVar(Name);
                if(rr != "NotFound"){
                    return rr;
                }
            }
            return "NotFound";
        }
        string GetUniqueName(string Name){
            if(symList.empty()){
                return "NotFound";
            }
            for(auto it = symList.end(); it!=symList.begin(); ){
                --it;
                string rr = (*it)->GetUniqueName(Name);
                if(rr != ""){
                    return rr;
                }
            }
            return "NotFound";
        }

        // Get whole information about a variable
        Symbol* GetInfo(string Name){
            if(symList.empty()){
                return nullptr;
            }
            for(auto it = symList.end(); it!=symList.begin(); ){
                --it;
                Symbol* rr = (*it)->GetInfo(Name);
                if(rr != nullptr){
                    return rr;
                }
            }
            return nullptr;
        }

        // This is a problematic function, 
        // because actually we cannot change a variables' value due to uncertainty
        // it is feasible before lv6 where if appears and the programme became uncertain
        int ChangeValue(string Name, int Value){ //used in assign
        //return the postfix of the unique_name of the variable it changed values.
            int i = symList.size();
            for(auto it = symList.end(); it!=symList.begin(); ){
                --it;
                int rt = (*it)->ChangeValue(Name,Value);
                if(rt == 1){
                    return i; // changed successfully
                    // this i is used to calculated the post_fix for the unique_name
                    // not useful now
                }
                i--;
            }
            return -1; // something wrong
        }



};

// Declared in ast.cpp
extern SymtabList glb_symtab_list; 


// 所有 AST 的基类
class BaseAST {
    public:
    // The Btype is an inhertied attribute
    // Declare it in Base so it can accessed by "upper" AST
    // This might causing memory waste, but currently I have no better idea
    // about how to deal with inherited attributes.
    // string BType;

    // vector used in repeat structures like repBlock or RepDecl
    // Inherited attributes, used in Rep__AST.
    unique_ptr<vector<unique_ptr<BaseAST>>> rep_vec_ptr; 

    // synthesized attributes, only used in ExprAST.
    bool expr_pure_value;
    string expr_result;
    ExpType opt;

    virtual ~BaseAST() = default;
    //在 C++ 定义的 AST 中, 我们可以借助虚函数的特性, 给 BaseAST 添加一个虚函数 Dump, 来输出 AST 的内容:
    virtual void Dump() const = 0;
};


// CompUnit 是 BaseAST
// Here the members of each class is the basic structure of the AST
// while the Code Generation process(IR code) is finished in Dump()
class CompUnitAST : public BaseAST {
    public:        
        std::unique_ptr<BaseAST> func_def;  

        void Dump() const override{
            //std::cout << "CompUnitAST { ";
            temp_var_used_for_load = 1000;
            func_def->Dump();
            //std::cout << " }";
            std::cout<<endl;
        }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
    public:
        std::unique_ptr<BaseAST> func_type;
        std::string ident; //function name
        std::unique_ptr<BaseAST> block;

        //为所有其他 AST 实现 Dump:
        void Dump() const override{
            // std::cout << "FuncDefAST { ";
            std::cout << "fun ";
            std::cout << "@" << ident << "()";
            func_type->Dump();
            std::cout << " {\n%entry:\n";
            block_ended = 0; //when ever enter a new block, set it as 0
            block->Dump();
            std::cout << "}";
        }
};

class FuncTypeAST : public BaseAST {
    public:
        string ftype;
        FuncTypeAST(string s):ftype(s){}
        void Dump() const override{
            cout<< ftype ;
        }   
};

class BlockAST : public BaseAST {
    public:
        std::unique_ptr<BaseAST> repBlockItems; // repeated block Items 
        void Dump() const override{
            if(repBlockItems == nullptr){
                cout<<"trying to dump something null"<<endl;
            }
            // cout<<"In BlockAST, line 195, call repBlockItems->Dump();"<<endl;
            repBlockItems->Dump();
        }
};

class RepBlockItemAST : public BaseAST {
    public:

        // unique_ptr<vector<unique_ptr<BaseAST>>> rep_vec_ptr;  //decalred in Base, because it's a inherited attribute

        // dump sentences.
        void Dump() const override {
            // cout<<"In repBlockAST, line 207, call BlockItemAST->Dump();"<<endl;
            if((*rep_vec_ptr).empty()){
                return;
            }
            else{
                // cout<< "line 212 *rep_vec_ptr.size(), should be 2" <<(*rep_vec_ptr).size()<<endl;
                for(auto i = (*rep_vec_ptr).begin(); i!= (*rep_vec_ptr).end(); ++i){
                    if((*i) == nullptr){
                        cout<<"error: tring to dump a nullptr"<<endl;
                    }
                    else{
                        // cout<< "line 218 try to dump a block item"<<endl;
                        if((*i).get()==nullptr){
                            cout<<"error: trying to dump but nothing is managed"<<endl;
                        }
                        (*i)->Dump();
                    }
                }
            }
        }
};

class BlockItemAST : public BaseAST {
    public:
        std::unique_ptr<BaseAST> sentc; // a sentence: could be a declaration or statements, etc
        // dump one sentences.
        void Dump() const override {
            //cout<<"In BlockItemAST, line 229, call VarDef/or stmt->Dump();"<<endl;
            sentc->Dump();
        }
};

class VarDeclAST : public BaseAST {
    //The problem here is that Btype is an inherited 
    public:
        std::unique_ptr<BaseAST> RepVarDeclAST; // repeated block Items 
        void Dump() const override{
            // cout<<"In VarDeclAST, line 240, call RepVarDeclAST->Dump();"<<endl;
            RepVarDeclAST->Dump();
        }

};

class RepVarDeclAST : public BaseAST {
    public:
    // This would be like RepBlock.

    // unique_ptr<vector<unique_ptr<BaseAST>>> rep_vec_ptr; 
    //decalred in Base, because it's a inherited attribute

    void Dump() const override {
        // cout<<"In RepVarDeclAST, line 254, call VarDeclAST->Dump();"<<endl;
        if((*rep_vec_ptr).empty()){
            return;
        }
        else{
            // cout<< "line 259 *rep_vec_ptr.size(), should be 1" <<(*rep_vec_ptr).size()<<endl;
            for(auto i = (*rep_vec_ptr).begin(); i!= (*rep_vec_ptr).end(); ++i){
                (*i)->Dump();
            }
        }
    }

};
/*
Working on:
// int x = 10;
@x = alloc i32
store 10, @x // dealing with duplication of name!!

VarDef ::= IDENT | IDENT "=" InitVal;
it's possible that the variable is declared but 
hasn't be initialized(given a value)

In stmt ,it may give the variable a value, and that will generate the
same code as :store 10, @x;

*/
class VarDefAST : public BaseAST { // dump one declaration
    public:
    // one variable declaration.
    // string Btype; // this is declared in the BaseAST.
    string name; 
    // int value;
    std::unique_ptr<BaseAST> expr; 
    bool initialized; // if the newly declared variable is initialized
    
    //Dealing with duplication of Name
    // The "name" should be passed to ast by parser when parsed 
    // a new variable. It look-up this name in symbtab, and if it is 
    // a duplication, it generate a new name for this variable
    VarDefAST(string s,bool x):name(s),initialized(x){} 

    void Dump() const override {
        if(initialized){
            // @x = alloc i32
            // store 10, @x; 
            if(!block_ended){ //wrap every ouput with a check
                expr->Dump();
                cout<<name<<" = alloc i32"<<endl;
                cout<<"store "<<expr->expr_result<<", "<<name<<endl;
            }
            
        }
        else{
            if(!block_ended){
                cout<<name<<" = alloc i32"<<endl;
            }
            
        }
    }

};

// Only need to write one kind of AST to generate calculating code
// because all the calculations are actually 3-addr-code
/*
enum class ExpType{
    _plus, //do nothing, or %k = add x, 0
    _minus, // %k = sub 0, x
    _not,  // %k = eq 0, x
    _add,  // %k = add x, y
    _sub,  // %k = sub x, y
    _mul,  // %k = mul x, y
    _div,  // %k = div x, y
    _mod,  // %k = mod x, y
    _lt, // <    // %k = lt x, y
    _gt, // >    // %k = gt x, y
    _le, // <=    // %k = le x, y
    _ge, // >=    // %k = ge x, y
    _eq,    // %k = eq x, y
    _neq,    // %k = ne x, y
    _and,    // %k = add x, y
    _or,    // %k = or x, y
    _Var,   
    _Number,
};

*/
class ExprAST : public BaseAST {
    public:
    // ExpType opt; //operation type
    // string expr_result; // the result can be const or a %k
    // if(pure_value == 1) then expre_result is a pure value(const number)

    // bool expr_pure_value; //if it is a pure value or the calculation process has already involes variables
    std::unique_ptr<BaseAST> operand1, operand2; 
    
    // cannot use dump to return anything.
    void Dump() const override{
        if(expr_pure_value){
            ; //generate no calculation code
        }
        else{
            // How to deal with this???
            // A. delete the const of Dump() and change all the Dump(), omit the const
            // B. change the parser.
            // expr_result = "%" + to_string(IR_temp_var_count);
            // cout<<"%"<<IR_temp_var_count<<" = ";
            cout<<"After parsing, IR_temp_var_count = "<<IR_temp_var_count<<endl;
            string result_1, result_2;
            /*
            if(opt == ExpType::_minus || opt == ExpType::_not || opt == ExpType::_plus){
                if(operand1->opt == ExpType::_Var){
                    result_1 = "%" + to_string(IR_temp_var_count++);
                    cout<<result_1<<" = load"<<operand1->expr_result<<endl;
                }
                result_1 = "%" + to_string(IR_temp_var_count);
            }
            else if (opt != ExpType::_Var && opt != ExpType::_Number ){
                result_1 = "%" + to_string(IR_temp_var_count++);
                cout<<result_1<<" = load"<<operand1->expr_result<<endl;
                result_2 = "%" + to_string(IR_temp_var_count++);
                cout<<result_2<<" = load"<<operand2->expr_result<<endl;
            }
            */
            switch(opt){
                case ExpType::_Var:
                    break;
                case ExpType::_Number:
                    break;
                case ExpType::_plus:
                    break;
                case ExpType::_minus:
                    operand1->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"sub "<< 0 <<", "<<result_1<<endl;
                    break;
                case ExpType::_not:
                    operand1->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"eq "<< 0 <<", "<<result_1<<endl;
                    break;
                case ExpType::_add:
                    operand1->Dump();
                    operand2->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    result_2 = operand2->expr_result;
                    if(operand2->opt == ExpType::_Var){
                        result_2 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_2<<" = load "<<operand2->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"add "<<result_1<<", "<<result_2<<endl;
                    break;
                case ExpType::_sub:
                    operand1->Dump();
                    operand2->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    result_2 = operand2->expr_result;
                    if(operand2->opt == ExpType::_Var){
                        result_2 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_2<<" = load "<<operand2->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"sub "<<result_1<<", "<<result_2<<endl;
                    break;
                case ExpType::_mul:
                    operand1->Dump();
                    operand2->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    result_2 = operand2->expr_result;
                    if(operand2->opt == ExpType::_Var){
                        result_2 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_2<<" = load "<<operand2->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"mul "<<result_1<<", "<<result_2<<endl;
                    break;
                case ExpType::_div:
                    operand1->Dump();
                    operand2->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    result_2 = operand2->expr_result;
                    if(operand2->opt == ExpType::_Var){
                        result_2 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_2<<" = load "<<operand2->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"div "<<result_1<<", "<<result_2<<endl;
                    break;
                case ExpType::_mod:
                    operand1->Dump();
                    operand2->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    result_2 = operand2->expr_result;
                    if(operand2->opt == ExpType::_Var){
                        result_2 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_2<<" = load "<<operand2->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"mod "<<result_1<<", "<<result_2<<endl;
                    break;
                case ExpType::_lt:
                    operand1->Dump();
                    operand2->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    result_2 = operand2->expr_result;
                    if(operand2->opt == ExpType::_Var){
                        result_2 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_2<<" = load "<<operand2->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"lt "<<result_1<<", "<<result_2<<endl;
                    break;
                case ExpType::_gt:
                    operand1->Dump();
                    operand2->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    result_2 = operand2->expr_result;
                    if(operand2->opt == ExpType::_Var){
                        result_2 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_2<<" = load "<<operand2->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"gt "<<result_1<<", "<<result_2<<endl;
                    break;
                case ExpType::_le:
                    operand1->Dump();
                    operand2->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    result_2 = operand2->expr_result;
                    if(operand2->opt == ExpType::_Var){
                        result_2 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_2<<" = load "<<operand2->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"le "<<result_1<<", "<<result_2<<endl;
                    break;
                case ExpType::_ge:
                    operand1->Dump();
                    operand2->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    result_2 = operand2->expr_result;
                    if(operand2->opt == ExpType::_Var){
                        result_2 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_2<<" = load "<<operand2->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"ge "<<result_1<<", "<<result_2<<endl;
                    break;
                case ExpType::_eq:
                    operand1->Dump();
                    operand2->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    result_2 = operand2->expr_result;
                    if(operand2->opt == ExpType::_Var){
                        result_2 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_2<<" = load "<<operand2->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"eq "<<result_1<<", "<<result_2<<endl;
                    break;
                case ExpType::_ne:
                    operand1->Dump();
                    operand2->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    result_2 = operand2->expr_result;
                    if(operand2->opt == ExpType::_Var){
                        result_2 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_2<<" = load "<<operand2->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"ne "<<result_1<<", "<<result_2<<endl;
                    break;
                case ExpType::_and:
                    operand1->Dump();
                    operand2->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    result_2 = operand2->expr_result;
                    if(operand2->opt == ExpType::_Var){
                        result_2 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_2<<" = load "<<operand2->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"and "<<result_1<<", "<<result_2<<endl;
                    break;
                case ExpType::_or:
                    operand1->Dump();
                    operand2->Dump();
                    result_1 = operand1->expr_result;
                    if(operand1->opt == ExpType::_Var){
                        result_1 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_1<<" = load "<<operand1->expr_result<<endl;
                    }
                    result_2 = operand2->expr_result;
                    if(operand2->opt == ExpType::_Var){
                        result_2 = "%" + to_string(IR_temp_var_count++);
                        cout<<result_2<<" = load "<<operand2->expr_result<<endl;
                    }
                    cout<<expr_result<<" = ";
                    cout<<"or "<<result_1<<", "<<result_2<<endl;
                    break;
                default:
                    break;
            }
            // IR_temp_var_count++;
        }
    }




};

class StmtAST : public BaseAST {
    public:
    /* Before Lv5:
        Stmt
            1. return ;  ==> ret 
            2. return Exp ; 
                for const value ==> ret const
                for variable: ==> %i = load %x \n ret %i  //为方便目标代码处理最好都这样load再用

            3. LVal "=" Exp ";" //赋值语句，should be given a name, and a value
            4. [Exp] ";" // 空语句，只做计算，忽略，不生成IR
            5. Block ； //递归，pass by a new *ast
     */
        StmtType stmt_type;
        // int val; //value of the expression.
        std::unique_ptr<BaseAST> expr; 
        string name;
        std::unique_ptr<BaseAST> block;  
    
        StmtAST(StmtType stt):stmt_type(stt){}

        void Dump() const override{
            string result;
            switch(stmt_type){
                case StmtType::_return:
                    if(!block_ended){
                        std::cout<<"ret "<<endl;
                        block_ended = 1;
                    }
                    break;
                case StmtType::_returnVar: //return Exp
                    if(!block_ended){
                        expr->Dump();//generate calculating code // or nothing if pure_value
                        // the result is a %k, or a pure_value
                        result = expr->expr_result;
                        if(expr->opt == ExpType::_Var){
                            result = "%" + to_string(IR_temp_var_count++);
                            cout<<result<<" = load "<<expr->expr_result<<endl;
                        }
                        std::cout<<"ret "<<result<<endl;
                        block_ended = 1; // whenever meet a ret/jmp/br, set is as 1
                    }
                    break;
                case StmtType::_assign: // store 10, @x
                    if(!block_ended){
                        expr->Dump();//generate calculating code
                        // the result is a %k
                        result = expr->expr_result;
                        if(expr->opt == ExpType::_Var){
                            result = "%" + to_string(IR_temp_var_count++);
                            cout<<result<<" = load "<<expr->expr_result<<endl;
                        }
                        std::cout<<"store "<<result<<", "<<name<<endl;
                    }
                    break;
                case StmtType::_block:
                    block->Dump();
                    break;
                case StmtType::_if:
                    block->Dump();
                    break;
                case StmtType::_while:
                    block->Dump();
                    break;
                case StmtType::_break:
                    if(!block_ended){
                        cout<<"jump "<<"%end_"<<while_stack.back()<<endl;
                        block_ended = 1;
                    }
                    break;
                case StmtType::_continue:
                    if(!block_ended){
                        cout<<"jump "<<"%while_entry_"<<while_stack.back()<<endl;
                        block_ended = 1;
                    }
                    break;
                
                default:
                    break;   
            }
        }
};

class IfThenElseAST : public BaseAST {
    public:
        std::unique_ptr<BaseAST> br_then, br_else; 
        // int cond;
        std::unique_ptr<BaseAST> expr; 
        bool empty_else;
        //IfThenElseAST(int x):cond(x){}
        void Dump() const override {
            //if entry
            label_count++; //enter scope, get_new_label:
            int k = label_count;
            end_stack.push_back(k);
            if(!block_ended){
                
                expr->Dump();
                cout<<"br "<<expr->expr_result<<", "<<"%then_"<<k<< ", "\
                <<"%else_" <<k<<endl;
                block_ended = 1;
            } 
            //enter a new block:
            if(!block_ended){ //eliminate adjoin label
                cout<<"jump "<<"%then_"<<k<<endl;
            }
            cout<<"%then_"<<k<<":"<<endl; 
            block_ended = 0;

            br_then->Dump();

            //enter a new block:
            if(!block_ended){ //eliminate adjoin label
                cout<<"jump "<<"%else_"<<k<<endl;
            }
            cout<<"%else_" <<k<<":"<<endl;
            block_ended = 0;

            br_else->Dump(); //could be empty

            if(!block_ended){
                cout<<"jump "<<"%end_"<<k<<endl;//end of else block, jump to end label
                block_ended = 1;
            }
            //enter a new block:
            if(!block_ended){ //eliminate adjoin label
                cout<<"jump "<<"%end_"<<k<<endl;
            }
            cout<<"%end_"<<k<<":"<<endl;
            block_ended = 0;

            end_stack.pop_back(); 
            //cout<<"end_stack pop!"<<endl;
            if(!end_stack.empty()){
                if(!block_ended){
                    cout<<"jump "<<"%end_"<< end_stack.back()<<endl; //jump to next end
                    block_ended = 1;
                }
                
            }
            
        }
};

class WhileAST : public BaseAST {
    public:
        std::unique_ptr<BaseAST> body; 
        std::unique_ptr<BaseAST> expr; 
        // int cond;
        // WhileAST(int x):cond(x){}
        
        void Dump() const override {
            label_count++; //enter scope, get_new_label:
            int k = label_count;
            end_stack.push_back(k);
            while_stack.push_back(k);

            //while_entry, enter new block
            if(!block_ended){ //eliminate adjoin label
                cout<<"jump "<<"%while_entry_"<<k<<endl;
            }
            cout<<"%while_entry_"<<k <<":"<<endl;
            block_ended = 0;//new block
            
            // The cond block
            expr->Dump();
            cout<<"br "<<expr->expr_result<<", %while_body_"<<k\
                <<", %end_"<<k<<endl;
            block_ended = 1;

            // New block: body:
            cout<<"%while_body_"<<k <<":"<<endl;
            block_ended = 0;//new block

            body->Dump(); //some calculation process
            cout<<"jump %while_entry_"<<k<<endl;
            block_ended = 1;
            

            //enter a new block:
            // if(!block_ended){ //eliminate adjoin label
            //     cout<<"jump "<<"%end_"<<k<<endl;
            // }
            cout<<"%end_"<<k <<":"<<endl;
            block_ended = 0;

            end_stack.pop_back(); 
            while_stack.pop_back();
            //cout<<"end_stack pop!"<<endl;
            if(!end_stack.empty()){
                if(!block_ended){
                    cout<<"jump "<<"%end_"<< end_stack.back()<<endl; 
                    block_ended = 1;
                }
            }
        }
};


class EmptyAST : public BaseAST { // empty ast for ';' etc.
    public:
        void Dump() const override {
            ;
        }
};

