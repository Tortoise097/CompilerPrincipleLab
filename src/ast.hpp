#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

// 所有 AST 的基类
class BaseAST {
    public:
    virtual ~BaseAST() = default;

    //在 C++ 定义的 AST 中, 我们可以借助虚函数的特性, 给 BaseAST 添加一个虚函数 Dump, 来输出 AST 的内容:
    virtual void Dump() const = 0;
};


// CompUnit 是 BaseAST

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
        std::string ident;
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
        std:: string func_type;
    FuncTypeAST(string f_type):func_type(f_type){}
    void Dump() const override{
        // std::cout << "FuncTypeAST { ";
        std::cout<< func_type ;
        // std::cout << " }";
    }
};

class BlockAST : public BaseAST {
    public:
        std::unique_ptr<BaseAST> stmt;
    void Dump() const override{
        //std::cout<<"BlockAST {";
        std::cout<<"ret ";
        stmt->Dump();
        //std::cout<<"}";
    }
};

class StmtAST : public BaseAST {
    public:
        int retv; //return value
    StmtAST(int r):retv(r){}
    void Dump() const override{
        //std::cout<<"StmtAST {";
        std::cout<<retv;
        //std::cout<<"}";
    }
};
