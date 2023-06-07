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
        string ftype;
        FuncTypeAST(string s):ftype(s){}
        void Dump() const override{
            cout<< ftype ;
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
        int exp_val; //value of the expression.
    
        StmtAST(StmtType stt):stmt_type(stt){}

        void Dump() const override{
        //std::cout<<"StmtAST {";
        std::cout<<"ret "<<exp_val;
        //std::cout<<"}";
    }
};


