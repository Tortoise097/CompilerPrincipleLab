#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

enum class StmtType{
    _return,
    _if,
    _else,
    _while,
    _decl, //a variable declaration
};

enum class DeclType{ //type of variable declaration
    _const,
    _var,
};

enum class VarType{
    _int,
};

enum class FuncType{
    _void,
    _int,
};

enum class OpType{ //operator's type
    // _plus, The Plus is ommitted by the parser
    _minus,
    _not,

    _add,
    _sub,
    _mul,
    _div,
    _mod,  //Koopa IR has the instruct "mod"

    _gr, // ">"
    _ls, // "<"
    _ge, // ">="
    _le, // "<="
    _eq, // "=="
    _neq, // "!="

    _and,  // "&&"
    _or,  // "||"
};

// 所有 AST 的基类
class BaseAST {
    public:
    /*  
    This Variable will be used by ExprAST
    It will hold the the name of the final temp variable it used 
    during the "calculating" process
    also I guess there will be sth wrong with the string here(potential memory leak )
    but I am too tired struggling with the smart pointer.
    so just let it be 
    */
    string retExp; // the "return" of those expr   
    
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
        std::cout << "@" << ident << "(): ";
        func_type->Dump();
        std::cout << " {\n%entry:\n";
        block->Dump();
        std::cout << "\n}";
    }
};

class FuncTypeAST : public BaseAST {
    public:
        FuncType func_type;
    FuncTypeAST(FuncType f_type):func_type(f_type){}
    void Dump() const override{
        if(func_type == FuncType::_int){
            cout<<"i32";
        }
    }
};

class BlockAST : public BaseAST {
    public:
    // this should be a list for multiple stmt,
    // but before lv4 here is only on statement: the return 
        std::unique_ptr<BaseAST> stmt;
    void Dump() const override{
        //std::cout<<"BlockAST {";
        stmt->Dump();
        //std::cout<<"}";
    }
};

class StmtAST : public BaseAST {
    public:
        StmtType stmt_type;
        std::unique_ptr<BaseAST> expr; //The actual statement
    
    StmtAST(StmtType stt):stmt_type(stt){}

    void Dump() const override{
        //std::cout<<"StmtAST {";
        expr->Dump();
        std::cout<<"ret ";
        std::cout<<expr->retExp;
        //std::cout<<"}";
    }
};

class ExprAST : public BaseAST{
    public:
    // string retExp;
    //Three types:
    //UnaryAST
    //AddAST
    //LOrAST
    std::unique_ptr<BaseAST> former; // the former calculated expr 
    string op1, op2 ; //operand1, operand2,
    // if Unary, then op2 will not be used.

    OpType op;  //operator;
    bool is_Number(string s){//test if the given string is a Number or a tempVariable
        if(s[0]=='%'){
            return 1;
        }
        else{
            return 0;
        }
    }

    int getTempVar(string s){//return a new temp var 
    // eg input %1 return %2
        int i = stoi(s)
    }
    
    void Dump() const override{
        former->Dump();
        switch(op){
            case OpType::_not :
            // !a ==> eq a, 0                
                retExp = 
                break;
            
            case OpType::_minus :
                break;

            case OpType::_add :
                break;
            
            case OpType::_sub :
                break;
            
            case OpType::_mul :
                break;
            
            case OpType::_div :
                break;
            
            case OpType::_mod :
                break;
            
            case OpType::_gr :
                break;
            
            case OpType::_ls :
                break;
            
            case OpType::_ge :
                break;
            
            case OpType::_le :
                break;

            case OpType::_eq :
                break;
            
            case OpType::_neq :
                break;
            
            case OpType::_and :
                break;
            
            case OpType::_or :
                break;

        }
    }

};

