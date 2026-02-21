#pragma once

struct VMCode{
    char segment0[10];
    char segment1[10];
    char segment2[16];
};

class VM{
public:

    static const int stack_length = 4092;
    static const int heap_length = 4092;
    static const int statics_length = 512;  
    static const int code_max_length = 4092;
    
    VMCode code[code_max_length];

    int stack[stack_length];    //memory
    int heap[heap_length];
    int statics[statics_length];
    int temp[8];

    int ip = 0; //instruction pointer;
    int sp = 0; //stack pointer
    int local_address = 0;
    int argument_address = 0;
    int this_address = 0;
    int that_address = 0;

    int code_last = 0;


    void run(const char* script, char *dst, int dst_size);

};