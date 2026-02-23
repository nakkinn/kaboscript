#include "virtual_machine.hpp"
#include "string_utils.hpp"


void VM::run(const char* script, char *dst, int dst_size){

    //初期化
    for(int i=0; i<stack_length; i++){
        stack[i] = 0;
    }
    for(int i=0; i<heap_length; i++){
        heap[i] = 0;
    }
    for(int i=0; i<statics_length; i++){
        statics[i] = 0;
    }
    for(int i=0; i<8; i++){
        temp[i] = 0;
    }

    stack[0] = stack_length;
    
    ip = 0;
    sp = 5;
    local_address = 5;
    argument_address = 0;
    this_address = 0;
    that_address = 0;

    code_last = 0;


    //codeの生成
    char seg0[10] = "";
    char seg1[64] = "";
    char seg2[16] = "";

    int flag1 = 0;     //0:seg0, 1:seg1, 2:seg2
    int j = 0;  //seg0,1,2のインデックス

    for(int i=0; i<my_strlen(script); i++){
        if(script[i] == '\n'){

            if(flag1==0)    seg0[j] = '\0';
            if(flag1==1)    seg1[j] = '\0';
            if(flag1==2)    seg2[j] = '\0';

            flag1 = 0;
            j = 0;

            if(seg0[0] != '\0' && seg0[0] != '#'){
                my_strcpy(seg0, code[code_last].segment0, 10);
                my_strcpy(seg1, code[code_last].segment1, 64);
                my_strcpy(seg2, code[code_last].segment2, 16);
                code_last++;
            }

            seg0[0] = '\0';
            seg1[0] = '\0';
            seg2[0] = '\0';

        }else if(script[i] == ' '){
            if(flag1==0)    seg0[j] = '\0';
            if(flag1==1)    seg1[j] = '\0';
            flag1++;
            j = 0;
        }else{
            if(flag1 == 0){
                if(j < 10-1){
                    seg0[j] = script[i];
                    j++;
                }
            }else if(flag1 == 1){
                if(j < 64-1){
                    seg1[j] = script[i];
                    j++;
                }
            }else if(flag1 == 2){
                if(j < 16-1){
                    seg2[j] = script[i];
                    j++;
                }
            }
            
        }
    }


    //ラベルの設定

    for(int i=0; i<code_last; i++){
        if( my_streq(code[i].segment0, "goto") || my_streq(code[i].segment0, "ifgo")){
            for(int j=0; j<code_last; j++){
                if( my_streq(code[j].segment0,"label") && my_streq(code[i].segment1,code[j].segment1)){
                    my_int2str(j, code[i].segment2, 12);
                    break;
                }
            }
        }
        if( my_streq(code[i].segment0, "call") ){
            for(int j=0; j<code_last; j++){
                if( my_streq(code[j].segment0,"function") && my_streq(code[i].segment1,code[j].segment1)){
                    my_int2str(j, code[i].segment1, 12);
                    break;
                }
            }
        }
    }


    //実行
    dst[0] = '\0';

    for(int k=0; k<999999; k++){

        if( my_streq(code[ip].segment0,"push") ){
            if(sp < stack_length-1){
                if(my_streq(code[ip].segment1,"argument")){
                    stack[sp] = stack[argument_address + my_str2int(code[ip].segment2)];
                }else if(my_streq(code[ip].segment1,"local")){
                    stack[sp] = stack[local_address + my_str2int(code[ip].segment2)];
                }else if(my_streq(code[ip].segment1,"static")){
                    stack[sp] = statics[my_str2int(code[ip].segment2)];
                }else if(my_streq(code[ip].segment1,"const")){
                    stack[sp] = my_str2int(code[ip].segment2);
                }else if(my_streq(code[ip].segment1,"this")){
                    stack[sp] = heap[this_address + my_str2int(code[ip].segment2)];
                }else if(my_streq(code[ip].segment1,"that")){
                    stack[sp] = heap[that_address + my_str2int(code[ip].segment2)];
                }else if(my_streq(code[ip].segment1,"pointer")){
                    if(code[ip].segment2[0]=='0')      stack[sp] = this_address;
                    else    stack[sp] = that_address;
                }else if(my_streq(code[ip].segment1,"temp")){
                    stack[sp] = temp[my_str2int(code[ip].segment2)];
                }
                sp++;
            }
        }else if( my_streq(code[ip].segment0,"pop")){
            if(sp >= 1){
                if(my_streq(code[ip].segment1, "argument")){
                    stack[argument_address + my_str2int(code[ip].segment2)] = stack[sp-1];
                }else if(my_streq(code[ip].segment1, "local")){
                    stack[local_address + my_str2int(code[ip].segment2)] = stack[sp-1];
                }else if(my_streq(code[ip].segment1, "static")){
                    statics[my_str2int(code[ip].segment2)] = stack[sp-1];
                }else if(my_streq(code[ip].segment1, "this")){
                    heap[this_address + my_str2int(code[ip].segment2)] = stack[sp-1];
                }else if(my_streq(code[ip].segment1, "that")){
                    heap[that_address + my_str2int(code[ip].segment2)] = stack[sp-1];
                }else if(my_streq(code[ip].segment1, "pointer")){
                    if(code[ip].segment2[0]=='0')  this_address = stack[sp-1];
                    else    that_address = stack[sp-1];
                }else if(my_streq(code[ip].segment1, "temp")){
                    temp[my_str2int(code[ip].segment2)] = stack[sp-1];
                }
                sp--;
            }

        }else if( my_streq(code[ip].segment0,"add") ){
            if(sp >= 2){
                stack[sp-2] += stack[sp-1];
                sp--;
            }
        }else if( my_streq(code[ip].segment0,"sub") ){
            if(sp >= 2){
                stack[sp-2] -= stack[sp-1];
                sp--;
            }
        }else if( my_streq(code[ip].segment0,"mul") ){
            if(sp >= 2){
                stack[sp-2] *= stack[sp-1];
                sp--;
            }
        }else if( my_streq(code[ip].segment0,"div") ){
            if(sp >= 2){
                stack[sp-2] /= stack[sp-1];
                sp--;
            }
        }else if( my_streq(code[ip].segment0,"mod") ){
            if(sp >= 2){
                stack[sp-2] %= stack[sp-1];
                sp--;
            }
        }else if( my_streq(code[ip].segment0,"neg") ){
            if( sp >= 1){
                stack[sp-1] *= -1;
            }
        }else if( my_streq(code[ip].segment0,"eq") ){
            if( sp >= 2){
                stack[sp-2] = stack[sp-2] == stack[sp-1];
                sp--;
            }
        }else if( my_streq(code[ip].segment0,"gt") ){
            if( sp >= 2){
                stack[sp-2] = stack[sp-2] > stack[sp-1];
                sp--;
            }
        }else if( my_streq(code[ip].segment0,"lt") ){
            if( sp >= 2){
                stack[sp-2] = stack[sp-2] < stack[sp-1];
                sp--;
            }
        }else if( my_streq(code[ip].segment0,"and") ){
            if( sp >= 2){
                stack[sp-2] = (stack[sp-2]!=0) && (stack[sp-1]!=0);
                sp--;
            }
        }else if( my_streq(code[ip].segment0,"or") ){
            if( sp >= 2){
                stack[sp-2] = (stack[sp-2]!=0) || (stack[sp-1]!=0);
                sp--;
            }
        }else if( my_streq(code[ip].segment0,"not") ){
            if( sp >= 1){
                stack[sp-1] = stack[sp-1]==0;
            }
        }else if( my_streq(code[ip].segment0,"out") ){
            if(sp >= 1){
                char tmp[12];
                my_int2str(stack[sp-1], tmp, 12);
                my_strcat(tmp, dst, dst_size);                
                sp--;
            }
        }else if( my_streq(code[ip].segment0,"outc") ){
            if(sp >= 1){
                char tmp[2];
                tmp[0] = stack[sp-1];
                tmp[1] = '\0';
                my_strcat(tmp, dst, dst_size);                
                sp--;
            }
        }else if( my_streq(code[ip].segment0,"outstr") ){
            my_strcat(code[ip].segment1, dst, dst_size);
        }else if( my_streq(code[ip].segment0, "outsp") ){
            char tmp[2];
            tmp[0] = ' ';
            tmp[1] = '\0';
            my_strcat(tmp, dst, dst_size);
        }else if( my_streq(code[ip].segment0, "outnl")){
            char tmp[2];
            tmp[0] = '\n';
            tmp[1] = '\0';
            my_strcat(tmp, dst, dst_size);
        }else if( my_streq(code[ip].segment0, "goto")){
            ip = my_str2int(code[ip].segment2);
        }else if( my_streq(code[ip].segment0, "ifgo")){
            if(sp >= 1){
                if(stack[sp-1]!=0)  ip = my_str2int(code[ip].segment2);
                sp--;
            }
        }else if( my_streq(code[ip].segment0, "call")){
            stack[sp] = ip + 1;
            stack[sp+1] = local_address;
            stack[sp+2] = argument_address;
            stack[sp+3] = this_address;
            stack[sp+4] = that_address;
            argument_address = sp - my_str2int(code[ip].segment2);
            local_address = sp + 5;
            sp += 5;
            ip = my_str2int(code[ip].segment1) - 1; //1つ進むとfunctionの位置
        }else if( my_streq(code[ip].segment0, "function")){
            sp += my_str2int(code[ip].segment2);
        }else if( my_streq(code[ip].segment0, "return")){
            int argument_address_tmp = argument_address;
            int return_address_tmp = stack[local_address-5];

            that_address = stack[local_address-1];  //ポインタを戻す
            this_address = stack[local_address-2];
            argument_address = stack[local_address-3];
            local_address = stack[local_address-4];

            stack[argument_address_tmp] = stack[sp-1];  //返り値の格納

            sp = argument_address_tmp + 1;
            ip = return_address_tmp - 1;

        }

        if(ip >= code_last-1)  break;
        ip++;
    }
}
