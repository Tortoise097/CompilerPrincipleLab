#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "ast.hpp"
using namespace std;

SymtabList glb_symtab_list; // declare a globle variable: The symbol table list

int label_count; //the if and while use the same label count
vector<int> while_stack;    // a stack used to keep track of which while we are in, used to deal with break and continue
vector<int> if_stack;
vector<int> end_stack;  //keep track of the "end" label, used in EndJump 
int IR_temp_var_count;

bool main_certain_return;
bool block_ended ; //if a block ended, which means no more jump or ret or any other insructions


string EndJumping(){ // special handling of Koopa's request of [the block must end with br/jmp/ret]
    if(end_stack.size() < 2){  //it is the outmost layer 
        return "";
    }
    else{
        auto it = end_stack.end();
        --it;
        --it;
        return "jump %end_" + to_string(*it);
    }
    return "something wrong in Endjumping\n";
}

void PrintStack(){//check the status of if stack and end stack
    cout<<"if_stack.size() = "<<if_stack.size()<<endl;
    cout<<"if_stack.back() = "<<if_stack.back()<<endl;
    cout<<"end_stack.size() = "<<end_stack.size()<<endl;
    cout<<"end_stack.back() = "<<end_stack.back()<<endl;  
}

// ...
