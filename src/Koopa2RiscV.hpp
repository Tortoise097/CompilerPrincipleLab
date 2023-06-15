#pragma once
#include "koopa.h"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include "ast.hpp"
using namespace std;

void Visit_raw_prog(const koopa_raw_program_t &program);
void Visit_slice(const koopa_raw_slice_t &slice);
void Visit_func(const koopa_raw_function_t &func);
void Visit_bbs(const koopa_raw_basic_block_t &bb);
void Visit_inst(const koopa_raw_value_t &value);
void Visit_inst_ret(const koopa_raw_return_t &i_ret);
void Visit_inst_int(const koopa_raw_integer_t &_integer);


void Koopa2RiscV(const char* str);