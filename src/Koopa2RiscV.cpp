#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include "ast.hpp"
#include "koopa.h"
using namespace std;


// 定义函数，以DFS的方式访问raw program
//function declarations:
void Visit_raw_prog(const koopa_raw_program_t &program);
void Visit_slice(const koopa_raw_slice_t &slice);
void Visit_func(const koopa_raw_function_t &func);
void Visit_bbs(const koopa_raw_basic_block_t &bb);
void Visit_inst(const koopa_raw_value_t &value);
void Visit_inst_ret(const koopa_raw_return_t &i_ret);
void Visit_inst_int(const koopa_raw_integer_t &_integer);

const int debug = 1;

// C++ function overloading
// 访问 raw program, #1
void Visit_raw_prog(const koopa_raw_program_t &program) {
    if(debug) cout<<"In Visit #1"<<endl;

    cout<<"  .text\n";
    cout<<"  .global main\n";

    // 访问所有全局变量 
    // Visit_slice(program.values);
    //no global variables declared currently
    
    // 访问所有函数
    if(debug) cout<<"In Visit #1, Visit functions"<<endl;
    Visit_slice(program.funcs); //visit global function definitions
    // --> typeof(program.fucs) is koopa_raw_slice_t, goto Visit #2
}

// 访问 raw slice, #2
void Visit_slice(const koopa_raw_slice_t &slice) {
    if(debug) cout<<"In Visit #2:visit_slice"<<endl;
    cout<<"slice.len = "<<slice.len<<endl;

    for (size_t i = 0; i < slice.len; ++i) {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        if(debug) cout<<"In Visit #2, slice.kind = "<<slice.kind<<endl;
        switch (slice.kind) {
        case KOOPA_RSIK_FUNCTION:
            // 访问函数
            if(debug) cout<<"In Visit #2, Visit functions"<<endl;
            Visit_func(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块
            if(debug) cout<<"In Visit #2, Visit Basic Blocks"<<endl;
            Visit_bbs(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令
            if(debug) cout<<"In Visit #2, Visit Instructions"<<endl;
            Visit_inst(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理
            if(debug) cout<<"In Visit #2,something wrong, It goes to default"<<endl;
            assert(false);
        }
    }
}

// 访问函数 #3
// The point is it didn't call Visit #3??? why???
void Visit_func(const koopa_raw_function_t &func) {
    if(debug) cout<<"In Visit #3:visit functions"<<endl;

    // declare function name:
    cout<<func->name<<endl;
    // 访问所有基本块
    Visit_slice(func->bbs);
}

// 访问基本块
void Visit_bbs(const koopa_raw_basic_block_t &bb) {
    if(debug) cout<<"In Visit #4:visit bbs"<<endl;
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  Visit_slice(bb->insts);
}

// 访问指令
void Visit_inst(const koopa_raw_value_t &value) {
    if(debug) cout<<"In Visit #5:visit instructions"<<endl;
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit_inst_ret(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit_inst_int(kind.data.integer);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

// 访问对应类型指令的函数定义略
// 视需求自行实现

// 访问 return 指令
void Visit_inst_ret(const koopa_raw_return_t &i_ret){ //return_instruction
    /*  li a0, ret_v      //load return value ret_v(an int) to register a0(which is used to store the return value)
        ret     //and return
    */
    if(debug) cout<<"In Visit #5: visit_inst_ret"<<endl;
    cout<<"  li a0, ";
    Visit_inst(i_ret.value); // call Visit #2, print the value
    cout<<"\n  ret"<<endl;
}

// 访问 integer 指令
// should this return an int?
void Visit_inst_int(const koopa_raw_integer_t &_integer){
    if(debug) cout<<"In Visit #6"<<endl;
    cout<<_integer.value;
}


void Koopa2RiscV(const char* str){
    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(str, &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    Visit_raw_prog(raw);
    // In my case(before lv8) the only function is main

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
}