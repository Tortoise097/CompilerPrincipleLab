#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include "ast.hpp"
#include "koopa.h"
using namespace std;

/*
    Program
        Global variable
            value 1
            value 2
        Function List
            Function 1
                BasicBlockList
                    BasicBlock 1
                        InstructionsList
                            Value 1 （Instruction)
                            Value 2  (Instruction)
                    BasicBlock 2
            Function 2

    structure of raw (koopa raw programme):
    koopa_raw_program_t{
        
    }
*/

// 定义函数，以DFS的方式访问raw program
// 函数声明略

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);

// C++ function overloading
// 访问 raw program
/* to be implemented
void Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有全局变量
  Visit(program.values);
  // 访问所有函数
  Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 访问函数
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
}

// 访问函数
void Visit(const koopa_raw_function_t &func) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有基本块
  Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...
*/

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
    // In my case(before lv8) the only function is main
    cout<<"  .text\n";
    cout<<"  .global main\n";
    
    // 使用 for 循环遍历函数列表
    for (size_t i = 0; i < raw.funcs.len; ++i) {  //只有main的情况raw.funcs.len就是1

        assert(raw.funcs.kind == KOOPA_RSIK_FUNCTION); //确认列表中的元素是函数

        // 获取当前函数
        koopa_raw_function_t func = (koopa_raw_function_t) raw.funcs.buffer[i];

        cout<<"main:\n"; 
        // 进一步处理当前函数：
        // bbs:basic block, 遍历函数里的BasicBlock
        // 这个basic block应该是指那些%label划分出来的基本块？
        for (size_t j = 0; j < func->bbs.len; ++j) {
            assert(func->bbs.kind == KOOPA_RSIK_BASIC_BLOCK);

            //获取当前基本块
            koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[j];
            
            //遍历基本块得到当前指令：
            for (size_t k = 0; k < bb->insts.len; ++k){
                assert(bb->insts.kind == KOOPA_RSIK_VALUE);
                // 进一步处理当前基本块
                koopa_raw_value_t value = (koopa_raw_value_t)bb->insts.buffer[k];
                // 示例程序中, 你得到的 value 一定是一条 return 指令
                assert(value->kind.tag == KOOPA_RVT_RETURN);
                // 于是我们可以按照处理 return 指令的方式处理这个 value
                // return 指令中, value 代表返回值
                koopa_raw_value_t ret_value = value->kind.data.ret.value;
                // 示例程序中, ret_value 一定是一个 integer
                assert(ret_value->kind.tag == KOOPA_RVT_INTEGER);
                // 于是我们可以按照处理 integer 的方式处理 ret_value
                // integer 中, value 代表整数的数值
                int32_t int_val = ret_value->kind.data.integer.value;
                // 示例程序中, 这个数值一定是 0
                assert(int_val == 0);
                cout<<"  li a0, 0\n"; 
                cout<<"  ret"; 
            }
        }


    }




    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
}