#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include "ast.hpp"
#include "Koopa2RiscV.hpp"
using namespace std;

// case 1: sysY --> koopa (file)  (0,)
// case 2: sysY --> koopa (console) (1,)
// case 3: sysY --> RiscV (file) (0,0)
// case 4: sysY --> RiscV (console) (0,1)

const int DEBUG_KOOPA = 0;
const int DEBUG_RISCV = 0;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern FILE *yyout;
extern int yyparse(unique_ptr<BaseAST> &ast);
extern void Koopa2RiscV(const char* str);


int main(int argc, const char *argv[]){
    // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
    // compiler 模式 输入文件 -o 输出文件
    assert(argc == 5);
    auto mode = argv[1];
    auto input = argv[2];
    auto output = argv[4];

    std::streambuf *psbuf, *backup;

    // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
    yyin = fopen(input, "r");
    assert(yyin);

    //generate .koopa file
    std::ofstream firout;
    if(DEBUG_KOOPA == 0){// redirect cout to the file "temp.koopa"
        firout.open ("temp.koopa"); //use temp.koopa to store generated IR code.
        backup = cout.rdbuf();  //keep a cout backup
        psbuf = firout.rdbuf();        // get file's streambuf
        std::cout.rdbuf(psbuf);         // assign streambuf to cout
    }
    
    // parse input file
    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast); // here the ast is passed as a reference
    assert(!ret);

    // generate Koopa IR code to temp.koopa
    ast->Dump();

    if(DEBUG_KOOPA == 0){
        std::cout.rdbuf(backup);        // restore cout's original streambuf
        firout.close();
    }
    
    // read in the contents of the IR file
    std::ifstream in("temp.koopa");
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    
    
    if(DEBUG_RISCV == 0){// Redirect cout to .S file.
        std::ofstream fasout; //assembly output
        fasout.open (output);
        backup = std::cout.rdbuf();     // back up cout's streambuf
        psbuf = fasout.rdbuf();        // get file's streambuf
        std::cout.rdbuf(psbuf);         // assign streambuf to cout
    
        Koopa2RiscV(contents.c_str()); //give the IR contents to Koopa2RiscV to process.
        
        std::cout.rdbuf(backup);        // restore cout's original streambuf
        fasout.close(); 
    }
    else{
        Koopa2RiscV(contents.c_str()); //directly output to console
    }
    

    return 0;
}
