//実行コマンド　g++ main.cpp string_utils.cpp virtual_machine.cpp -o main && main

#include <iostream>

#include "string_utils.hpp"
#include "virtual_machine.hpp"

const char* vm_header = R"(
call main 0
pop temp 0
goto $end
)";

const char* vm_footer = R"(
function print 0
push argument 0
out
outsp
push const 0
return

function newline 0
outnl
push const 0
return

function length 0
push argument 0
pop pointer 1
push that 0
return

function $alloc 0
push temp 7
pop temp 0
push temp 7
push argument 0
add
pop temp 7
push temp 0
return

label $end
)";


const char* jack_script0 = R"(

function max_val(arr){
    var best = arr[0];
    for(var i=0; i<length(arr); i++){
        if(arr[i] > best)   best = arr[i];
    }
    return best;
}

function main(){
    var data[5];
    data[0] = 3;
    data[1] = 37;
    data[2] = 1;
    data[3] = 9;
    data[4] = 5;
    print( max_val(data) );
    return 0;
}


)";

char jack_script[1000];




void remove_comment(){

    //各行で // 移行を除去
    bool flag1 = false;
    for(int i=0; i<my_strlen(jack_script); i++){
        if(jack_script[i]=='/' && jack_script[i+1]=='/' && !flag1)  flag1 = true;
        if(jack_script[i]=='\n')    flag1 = false;
        if(flag1)   jack_script[i] = ' ';
    }

    // /* */ の削除
    flag1 = false;
    for(int i=0; i<my_strlen(jack_script); i++){
        if(jack_script[i]=='/' && jack_script[i+1]=='*' && !flag1)  flag1 = true;
        if(flag1)   jack_script[i] = ' ';
        if(i>0) if(jack_script[i-1]=='*' && jack_script[i]=='/' && flag1)    flag1 = false;
    }

}



int tokenize_index = 0;
int error_line = -1;


//トークンインデックスからjack_scriptの行数を返す
int line_of_token(int index){
    int result = 0;
    for(int i=0; i<index; i++){
        if(jack_script[i] == '\n')  result++;
    }
    return result + 1;
}

//行数からjack_scriptのその行の文字列を返す
void source_of_line(int line, char* dst, int dst_size){
    dst[0] = '\0';
    int i = 1;  //行数
    int j = 0;  //何文字目
    int k = 0;  //指定行で何文字目

    //指定行まで
    while(jack_script[j]!='\0'){
        if(i == line)   break;
        if(jack_script[j] == '\n')  i++;
        j++;
    }

    while(jack_script[j]!='\0' && jack_script[j]!='\n' && k<dst_size-1){
        dst[k++] = jack_script[j++];
    }
    dst[k] = '\0';
} 

void nextToken(char* dst, int dst_size, bool commit){

    int i = tokenize_index;
    int j = 0;

    //空白を飛ばす
    while( jack_script[i] && is_space(jack_script[i]))  i++;

    //終端
    if(jack_script[i]=='\0'){
        dst[0] = '\0';
        if(commit)  tokenize_index = i;
        return;
    }

    //数値
    if(is_digit(jack_script[i])){
        while( jack_script[i] && is_digit(jack_script[i]) && j<dst_size-1 ){
            dst[j++] = jack_script[i++];
        }
        dst[j] = '\0';
        if(commit)  tokenize_index = i;
        return;
    }

    //識別子
    if(is_ident_head(jack_script[i])){
        while( jack_script[i] && is_ident_tail(jack_script[i]) && j<dst_size-1){
            dst[j++] = jack_script[i++];
        }
        dst[j] = '\0';
        if(commit)  tokenize_index = i;
        return;
    }

    //2文字記号
    if( (jack_script[i]=='<' && jack_script[i+1]=='=') || (jack_script[i]=='>' && jack_script[i+1]=='=') || (jack_script[i]=='=' && jack_script[i+1]=='=') ||
    (jack_script[i]=='!' && jack_script[i+1]=='=') || (jack_script[i]=='&' && jack_script[i+1]=='&') || (jack_script[i]=='|' && jack_script[i+1]=='|') ||
    (jack_script[i]=='+' && jack_script[i+1]=='+') || (jack_script[i]=='-' && jack_script[i+1]=='-') || (jack_script[i]=='+' && jack_script[i+1]=='=') || 
    (jack_script[i]=='-' && jack_script[i+1]=='=') || (jack_script[i]=='*' && jack_script[i+1]=='=') ){
        dst[0] = jack_script[i];
        dst[1] = jack_script[i+1];
        dst[2] = '\0';
        if(commit)  tokenize_index = i + 2;
        return;
    }

    //1文字記号
    dst[0] = jack_script[i];
    dst[1] = '\0';
    if(commit)  tokenize_index = i + 1;
}


void nnextToken(char* dst, int dst_size){
    int tmp = tokenize_index;
    nextToken(dst, dst_size, true);
    nextToken(dst, dst_size, false);
    tokenize_index = tmp;
}

void expect(const char* src){
    if(error_line == -1){
        int tokenize_index_copy = tokenize_index;
        char tmp[64];
        nextToken(tmp, 64, true);
        if( !my_streq(src,tmp) ){
            std::cout << "debug1:" << jack_script[tokenize_index] << tokenize_index << std::endl;
            error_line = line_of_token(tokenize_index);
        }
    }
}





enum NodeType{
    NODE_FUNC,      //left:name_id, right:return_type, third:arg_first, fourth:arg_count, local_count, body, next
    NODE_ARG,       //left:var_id, next
    NODE_INT,       //left:value
    NODE_VARREF,
    NODE_INDEX,     //left:var_id, right:index, next
    NODE_BINARY,
    NODE_UNARY,
    NODE_VARDEC,    //left:var_id, right:init_expr, third:is_array, fourth:array_size, next
    NODE_VARSET,    //left:var_id, right:expr, next
    NODE_IF,        //left:condition, right:then_head_id, third:else_head_id, next
    NODE_WHILE,     //left:condition, right:then_head_id, next
    NODE_FOR,       //left:init, right:cond, third:update, foruth:statement, next
    NODE_CALL,      //left:func_id, right:arg_head_id, third:arg_count, next
    NODE_BREAK,     //next
    NODE_RETURN     //left:expr, next
};

enum Op{
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_NEG,
    OP_NOT,
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE,
    OP_EQ,
    OP_NE,
    OP_AND,
    OP_OR
};

struct Node{
    NodeType type;
    Op op;
    int left;
    int right;
    int third;
    int fourth;
    int local_count;
    int body;
    int next;
    // char value[11];
};



static const int node_max = 512;
int node_index = 0;
Node nodes[node_max];

int name_table_max = 512;
int name_table_index = 0;
char name_table[512][64];

int local_count, current_local;






void initialize_name_table(){
    for(int i=0; i<name_table_max; i++){
        name_table[i][0] = '\0';
    }
    name_table_index = 0;
}

int set_name_table(const char* src){

    //既に登録済み
    for(int i=0; i<name_table_index; i++){
        if(my_streq(src, name_table[i]))    return i;
    }

    //新規登録
    my_strcpy( src, name_table[name_table_index++], 64);
    return name_table_index - 1;
}

int ref_name_table(const char* src){
    for(int i=0; i<name_table_index; i++){
        if(my_streq(src, name_table[i]))    return i;
    }
    return -1;
}


int new_func(int id, int return_type, int arg_first, int arg_count, int local_count, int body){
    int idx = node_index++;
    nodes[idx].type = NODE_FUNC;
    nodes[idx].left = id;
    nodes[idx].right = return_type;
    nodes[idx].third = arg_first;
    nodes[idx].fourth = arg_count;
    nodes[idx].local_count = local_count;
    nodes[idx].body = body;
    nodes[idx].next = -1;
    return idx;
}

int new_vardec(int id, int init, int is_array, int array_size){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_VARDEC;
    nodes[idx].left = id;
    nodes[idx].right = init;
    nodes[idx].third = is_array;
    nodes[idx].fourth = array_size;
    nodes[idx].next = -1;
    return idx;
}

int new_argdec(int id){
    int idx = node_index++;
    nodes[idx].type = NODE_ARG;
    nodes[idx].left = id;
    nodes[idx].next = -1;
    return idx;
}

int new_varset(int id, int expr){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_VARSET;
    nodes[idx].left = id;
    nodes[idx].right = expr;
    nodes[idx].next = -1;
    return idx;
}

int new_if(int cond, int then_index, int else_index){
    int idx = node_index++;
    nodes[idx].type = NODE_IF;
    nodes[idx].left = cond;
    nodes[idx].right = then_index;
    nodes[idx].third = else_index;
    nodes[idx].next = -1;
    return idx;
}

int new_while(int cond, int then_index){
    int idx = node_index++;
    nodes[idx].type = NODE_WHILE;
    nodes[idx].left = cond;
    nodes[idx].right = then_index;
    nodes[idx].next = -1;
    return idx;
}

int new_for(int init, int cond, int update, int stmt){
    int idx = node_index++;
    nodes[idx].type = NODE_FOR;
    nodes[idx].left = init;
    nodes[idx].right = cond;
    nodes[idx].third = update;
    nodes[idx].fourth = stmt;
    nodes[idx].next = -1;
    return idx;
}

int new_call(int id, int arg_index, int arg_count){
    int idx = node_index++;
    nodes[idx].type = NODE_CALL;
    nodes[idx].left = id;
    nodes[idx].right = arg_index;
    nodes[idx].third = arg_count;
    nodes[idx].next = -1;
    return idx;
}

int new_break(){
    int idx = node_index++;
    nodes[idx].type = NODE_BREAK;
    nodes[idx].next = -1;
    return idx;
}

int new_return(int expr){
    int idx = node_index++;
    nodes[idx].type = NODE_RETURN;
    nodes[idx].left = expr;
    nodes[idx].next = -1;
    return idx;
}

int new_binary(Op op, int left, int right){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_BINARY;
    nodes[idx].op = op;
    nodes[idx].left = left;
    nodes[idx].right = right;
    nodes[idx].next = -1;
    return idx;
}

int new_unary(Op op, int left){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_UNARY;
    nodes[idx].op = op;
    nodes[idx].left = left;
    nodes[idx].right = -1;
    nodes[idx].next = -1;
    return idx;
}

int new_int(int value){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_INT;
    nodes[idx].left = value;
    nodes[idx].next = -1;
    return idx;
}

int new_varref(int id){
    int idx = node_index++;
    nodes[idx].type = NODE_VARREF;
    nodes[idx].left = id;
    nodes[idx].next = -1;
    return idx;
}

int new_index(int varid, int index){
    int idx = node_index++;
    nodes[idx].type = NODE_INDEX;
    nodes[idx].left = varid;
    nodes[idx].right = index;
    nodes[idx].next = -1;
    return idx;
}



int parse_main();

int parse_func();

int parse_statement_list();
int parse_statement();

int parse_vardec();
int parse_varset();
int parse_if();
int parse_while();
int parse_for();
int parse_call();
int parse_break();
int parse_return();
int parse_exprstmt();

int parse_expr();
int parse_logical_or();
int parse_logical_and();
int parse_equality();
int parse_relational();
int parse_add_sub();
int parse_mul_div();
int parse_unary();
int parse_postfix();
int parse_primary();


int parse_main(){
    if(error_line != -1)    return -1;
    initialize_name_table();

    char tmp[64];

    int origin=-1, node=-1, node0=-1;


    nextToken(tmp, 64, false);  //function procedure var確認

    while(tmp[0]!='\0'){
        int prev = tokenize_index;
        if( my_streq("function",tmp) || my_streq("procedure",tmp)){
            node = parse_func();
        }else if(my_streq("var",tmp)){
            node = parse_vardec();
            expect(";");
        }else{
            nextToken(tmp, 64, true);   //tokenize_indexを進める
            // syntax_error = true;
            error_line = line_of_token(tokenize_index);
        }
        
        if(prev == tokenize_index)  error_line = line_of_token(tokenize_index);

        if(error_line != -1)    return -1;
        
        if(origin==-1)  origin = node;
        if(node0!=-1)   nodes[node0].next = node;
        node0 = node;

        nextToken(tmp, 64, false);
    }

    return origin;

}

int parse_func(){
    if(error_line != -1)    return -1;
    char tmp[64];
    int return_type=0, func_id, body_id;
    int arg_id=-1, arg_id0=-1, arg_origin=-1, arg_count=0;

    current_local = 0;
    local_count = 0;
    
    nextToken(tmp, 64, true);   //function or procedure
    if( my_streq("function", tmp) ) return_type = 1;

    nextToken(tmp, 64, true);   //関数名
    func_id = set_name_table(tmp);

    expect("(");

    nextToken(tmp, 64, false);

    while(tmp[0]!='\0' && tmp[0]!=')'){
        if(arg_origin!=-1)  expect(",");
        nextToken(tmp, 64, true);
        arg_id = new_argdec( set_name_table(tmp) );
        if(arg_origin == -1) arg_origin = arg_id; 
        if(arg_id0 != -1)   nodes[arg_id0].next = arg_id;   //1つ前のarg
        arg_id0 = arg_id;
        nextToken(tmp, 64, false);
        arg_count++;
    }

    expect(")");
    expect("{");
    body_id = parse_statement_list();
    expect("}");
    
    return new_func(func_id, return_type, arg_origin, arg_count, local_count, body_id);
}


int parse_statement_list(){
    if(error_line != -1)    return -1;
    char tmp[64];
    int node0 = -1, node1 = 0, node_origin = -1;
    int current_local_copy = current_local;
    
    nextToken(tmp, 64, false);
    

    while( node1 != -1 && tmp[0]!='}'){
        int prev = tokenize_index;
        node1 = parse_statement();
        if(tokenize_index == prev)  error_line = line_of_token(tokenize_index);

        if(error_line != -1)    return -1;

        if(node1!=-1){
            if(node0 == -1) node_origin = node1;    //先頭ノード
            else    nodes[node0].next = node1;  //1つ前
            node0 = node1;
        }
        nextToken(tmp, 64, false);
    }

    current_local = current_local_copy;
    return node_origin;
}

int parse_statement(){
    if(error_line != -1)    return -1;
    char tmp1[64];
    char tmp2[64];
    nextToken(tmp1, 64, false);
    nnextToken(tmp2, 64);

    int node = -1;
    
    if( my_streq(tmp1, "var") ){
        node = parse_vardec();
        expect(";");
    }else if( my_streq(tmp1, "if") ){
        node = parse_if();
    }else if( my_streq(tmp1, "while") ){
        node = parse_while();
    }else if( my_streq(tmp1, "for") ){
        node = parse_for();
    }else if( my_streq(tmp1, "break") ){
        node = parse_break();
    }else if( my_streq(tmp1, "return") ){
        node = parse_return();
    }else if( tmp2[0] == '('){
        node = parse_call();    
        expect(";");
    }else if( tmp1[0] != '\0'){
        node = parse_varset();
        expect(";");
    }
    
    return node;
}

int parse_vardec(){
    if(error_line != -1)    return -1;
    char tmp[64];
    int id;
    int init = -1;
    int is_array = 0;
    int array_size = 0;

    current_local++;
    if(current_local > local_count) local_count = current_local;

    nextToken(tmp, 64, true);  //var
    nextToken(tmp, 64, true);  // x 
    id = set_name_table(tmp);

    nextToken(tmp, 64, false);  // [ 確認
    if(tmp[0] == '['){
        expect("[");
        is_array = 1;
        nextToken(tmp, 64, true);   //数値
        array_size = my_str2int(tmp);
        expect("]");
    }

    nextToken(tmp, 64, false);  // = 確認
    if(tmp[0]=='='){ 
        expect("=");
        init = parse_expr();
    }
    return new_vardec(id, init, is_array, array_size);
}

int parse_varset(){
    if(error_line != -1)    return -1;
    char tmp[64];
    int id;
    int value;

    id = parse_expr();
    
    nextToken(tmp, 64, false);  // = ++ -- += -= *=

    if(tmp[0]=='='){
        nextToken(tmp, 64, true);   // = 消費
        value = parse_expr();
    }else if(tmp[0]=='+' && tmp[1]=='+'){
        nextToken(tmp, 64, true);   // ++ 消費
        value = new_binary(OP_ADD, id, new_int(1));
    }else if(tmp[0]=='-' && tmp[1]=='-'){
        nextToken(tmp, 64, true);
        value = new_binary(OP_SUB, id, new_int(1));
    }else if(tmp[0]=='+' && tmp[1]=='='){
        nextToken(tmp, 64, true);
        value = new_binary(OP_ADD, id, parse_expr());
    }else if(tmp[0]=='-' && tmp[1]=='='){
        nextToken(tmp, 64, true);
        value = new_binary(OP_SUB, id, parse_expr());
    }else if(tmp[0]=='*' && tmp[1]=='='){
        nextToken(tmp, 64, true);
        value = new_binary(OP_MUL, id, parse_expr());
    }else{
        // syntax_error = true;
        error_line = line_of_token(tokenize_index);
    }
    
    return new_varset(id, value);
}

int parse_if(){
    if(error_line != -1)    return -1;
    char tmp[64];
    int cond = -1;
    int then_index = -1;
    int else_index = -1;
    expect("if");

    expect("(");
    cond = parse_expr();
    expect(")");

    nextToken(tmp, 64, false);
    if( tmp[0] == '{'){
        expect("{");
        then_index = parse_statement_list();
        expect("}");
    }else{
        then_index = parse_statement();
    }
    
    nextToken(tmp, 64, false);  // else確認
    if(my_streq(tmp,"else")){
        nextToken(tmp, 64, true);  //else消費
        nextToken(tmp, 64, false); // { 確認
        if( tmp[0] == '{'){
            expect("{");
            else_index = parse_statement_list();
            expect("}");
        }else{
            else_index = parse_statement();
        }
    }

    return new_if(cond, then_index, else_index);
}

int parse_while(){
    if(error_line != -1)    return -1;
    char tmp[64];
    int cond = -1;
    int then_index = -1;

    nextToken(tmp, 64, true);  //while

    expect("(");
    cond = parse_expr();
    expect(")");

    nextToken(tmp, 64, false); // { 確認
    if( tmp[0] == '{'){
        expect("{");
        then_index = parse_statement_list();
        expect("}"); 
    }else{
        then_index = parse_statement();
    }

    return new_while(cond, then_index);
}

int parse_for(){
    if(error_line != -1)    return -1;
    char tmp[64];
    int init = 1;
    int cond = -1;
    int update = -1;
    int stmt = -1;

    expect("for");
    expect("(");

    nextToken(tmp, 64, false);  //確認
    if(tmp[0]!=';') init = parse_vardec(); 
    expect(";");

    nextToken(tmp, 64, false);  //確認
    if(tmp[0]!=';') cond = parse_expr();
    expect(";");

    nextToken(tmp, 64, false);  //確認
    if(tmp[0]!=';') update = parse_varset();;
    expect(")");
    
    nextToken(tmp, 64, false); // { 確認
    if( tmp[0] == '{'){
        expect("{");
        stmt = parse_statement_list();
        expect("}");
    }else{
        stmt = parse_statement();
    }

    return new_for(init, cond, update, stmt);
}

int parse_call(){
    if(error_line != -1)    return -1;
    char tmp[64];
    int id = -1;
    int arg_index0 = -1;
    int arg_index1 = -1;
    int arg_origin = -1;
    int arg_count = 0;

    nextToken(tmp, 64, true);   //関数名
    id = set_name_table(tmp);   //ref_name_table
    expect("(");

    nextToken(tmp, 64, false);
    while( tmp[0]!='\0' && tmp[0]!=')'){
        if(arg_count != 0)  expect(",");
        arg_index1 = parse_expr();
        if(arg_count == 0)  arg_origin = arg_index1;
        else    nodes[arg_index0].next = arg_index1;
        arg_index0 = arg_index1;
        arg_count++;
        nextToken(tmp, 64, false);
    }

    expect(")");
    
    return new_call(id, arg_origin, arg_count);
}

int parse_break(){
    if(error_line != -1)    return -1;
    expect("break");
    expect(";");
    return new_break();
}

int parse_return(){
    if(error_line != -1)    return -1;
    expect("return");
    int expr = parse_expr();
    expect(";");
    return new_return(expr);
}

int parse_expr(){
    if(error_line != -1)    return -1;
    return parse_logical_or();
}

int parse_logical_or(){
    if(error_line != -1)    return -1;
    int left = parse_logical_and();
    char tmp[64];
    nextToken(tmp, 64, false); //進めずに確認
    while( my_streq(tmp,"||") ){
        nextToken(tmp, 64, true);    //1つ進める
        int right = parse_logical_and();
        left = new_binary(OP_OR, left, right);
        nextToken(tmp, 64, false);
    }
    return left;
}

int parse_logical_and(){
    if(error_line != -1)    return -1;
    int left = parse_equality();
    char tmp[64];
    nextToken(tmp, 64, false); //進めずに確認
    while( my_streq(tmp,"&&") ){
        nextToken(tmp, 64, true);    //1つ進める
        int right = parse_equality();
        left = new_binary(OP_AND, left, right);
        nextToken(tmp, 64, false);
    }
    return left;
}

int parse_equality(){
    if(error_line != -1)    return -1;
    int left = parse_relational();

    char tmp[64];
    nextToken(tmp, 64, false); //進めずに確認
    if( my_streq(tmp,"==") || my_streq(tmp,"!=") ){
        nextToken(tmp, 64, true);    //1つ進める
        int right = parse_relational();
        if( my_streq(tmp,"==") )    left = new_binary(OP_EQ, left, right);
        if( my_streq(tmp,"!=") )    left = new_binary(OP_NE, left, right);      
    }
    
    return left;
}

int parse_relational(){
    if(error_line != -1)    return -1;
    int left = parse_add_sub();

    char tmp[64];
    nextToken(tmp, 64, false); //進めずに確認
    if( my_streq(tmp,">") || my_streq(tmp,"<") || my_streq(tmp,">=") || my_streq(tmp,"<=")){
        nextToken(tmp, 64, true);    //1つ進める
        int right = parse_add_sub();
        if( my_streq(tmp,">") )    left = new_binary(OP_GT, left, right);
        if( my_streq(tmp,"<") )    left = new_binary(OP_LT, left, right);
        if( my_streq(tmp,">=") )    left = new_binary(OP_GE, left, right);
        if( my_streq(tmp,"<=") )    left = new_binary(OP_LE, left, right);        
    }
    
    return left;
}

int parse_add_sub(){
    if(error_line != -1)    return -1;
    int left = parse_mul_div();

    char tmp[64];
    nextToken(tmp, 64, false); //進めずに確認
    while( my_streq(tmp,"+") || my_streq(tmp,"-") ){
        nextToken(tmp, 64, true);    //1つ進める
        int right = parse_mul_div();
        Op op;
        if( my_streq(tmp,"+") )    op = OP_ADD;
        if( my_streq(tmp,"-") )    op = OP_SUB;
        left = new_binary(op, left, right);
        nextToken(tmp, 64, false);
    }
    
    return left;
}

int parse_mul_div(){
    if(error_line != -1)    return -1;
    int left = parse_unary();

    char tmp[64];
    nextToken(tmp, 64, false); //進めずに確認
    while( my_streq(tmp,"*") || my_streq(tmp,"/") || my_streq(tmp,"%")){
        nextToken(tmp, 64, true);    //1つ進める
        int right = parse_unary();
        Op op;
        if( my_streq(tmp,"*") )    op = OP_MUL;
        if( my_streq(tmp,"/") )    op = OP_DIV;
        if( my_streq(tmp,"%") )    op = OP_MOD;

        left = new_binary(op, left, right);
        nextToken(tmp, 64, false);
    }
    
    return left;
}

int parse_unary(){
    if(error_line != -1)    return -1;
    char tmp[64];
    nextToken(tmp, 64, false); //進めずに確認
    int node = -1;

    if( tmp[0]=='+' ){
        nextToken(tmp, 64, true);    // + を消費
        node = parse_primary();
    }else if( tmp[0] == '-'){
        nextToken(tmp, 64, true);    // - を消費
        node = parse_primary();
        node = new_unary( OP_NEG, node );
    }else if( tmp[0] == '!'){
        nextToken(tmp, 64, true);    // ! を消費
        node = parse_primary();
        node = new_unary( OP_NOT, node);
    }else{
        node = parse_primary();
    }
    
    return node;
}


int parse_primary(){
    if(error_line != -1)    return -1;
    char tmp1[64];
    char tmp2[64];
    nextToken(tmp1, 64, false);
    nnextToken(tmp2, 64);
    int node = -1;

    if( is_digit(tmp1[0]) ){ //数値
        nextToken(tmp1, 64, true);
        node = new_int(my_str2int(tmp1));
    }else if( tmp1[0] == '(' ){  //式
        nextToken(tmp1, 64, true);    // ( を消費
        node = parse_expr();
        nextToken(tmp1, 64, true);    // ) を消費
    }else if( tmp2[0] == '('){  //関数呼び出し
        node = parse_call();
    }else if( is_ident_head(tmp1[0]) ){  //変数
        nextToken(tmp1, 64, true);
        node = new_varref( set_name_table(tmp1) );  //ref_name_table 
        if(tmp2[0] == '['){
            nextToken(tmp1, 64, true); // [ を消費
            node = new_index(node, parse_expr());
            nextToken(tmp1, 64, true);  // ]を消費
        }
    }else{
        error_line = line_of_token(tokenize_index);
    }
    
    return node;
}







//デバッグ出力

void print_main(int index, int nest);
void print_func(int index, int nest);
void print_binary(int index, int nest);
void print_unary(int index, int nest);
void print_int(int index, int nest);
void print_argdec(int index, int nest);
void print_vardec(int index, int nest);
void print_varset(int index, int nest);
void print_varref(int index, int nest);
void print_index(int index, int nest);
void print_if(int index, int nest);
void print_while(int index, int nest);
void print_for(int index, int nest);
void print_call(int index, int nest);
void print_break(int index, int nest);
void print_return(int index, int nest);


void print_main(int index, int nest){
    if(index == -1) return;
    if(nodes[index].type == NODE_FUNC)  print_func(index, nest);
    if(nodes[index].type == NODE_BINARY)    print_binary(index, nest);
    if(nodes[index].type == NODE_UNARY)    print_unary(index, nest);
    if(nodes[index].type == NODE_INT)    print_int(index, nest);
    if(nodes[index].type == NODE_VARDEC)    print_vardec(index, nest);
    if(nodes[index].type == NODE_VARSET)    print_varset(index, nest);
    if(nodes[index].type == NODE_VARREF)    print_varref(index, nest);
    if(nodes[index].type == NODE_INDEX)    print_index(index, nest);
    if(nodes[index].type == NODE_IF)    print_if(index, nest);
    if(nodes[index].type == NODE_WHILE)    print_while(index, nest);
    if(nodes[index].type == NODE_FOR)    print_for(index, nest);
    if(nodes[index].type == NODE_CALL)    print_call(index, nest);
    if(nodes[index].type == NODE_ARG)    print_argdec(index, nest);
    if(nodes[index].type == NODE_BREAK)    print_break(index, nest);
    if(nodes[index].type == NODE_RETURN)    print_return(index, nest);
}

void print_func(int index, int nest){
    char output[128] = "";
    char tmp[12];
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("FUNC(", output, 128);
    my_int2str(nodes[index].left, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(",", output, 128);
    my_int2str(nodes[index].fourth, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(",", output, 128);
    my_int2str(nodes[index].local_count, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].third, nest+1);
    print_main(nodes[index].body, nest+1);

    print_main(nodes[index].next, nest);
}

void print_argdec(int index, int nest){
    char output[128] = "";
    char tmp[12];
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("ARG(", output, 128);
    my_int2str(nodes[index].left, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].next, nest);
}

void print_binary(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("BIN(", output, 128);
    if(nodes[index].op == OP_ADD)   my_strcat("ADD", output, 128);
    if(nodes[index].op == OP_SUB)   my_strcat("SUB", output, 128);
    if(nodes[index].op == OP_MUL)   my_strcat("MUL", output, 128);
    if(nodes[index].op == OP_DIV)   my_strcat("DIV", output, 128);
    if(nodes[index].op == OP_MOD)   my_strcat("MOD", output, 128);
    if(nodes[index].op == OP_LT)   my_strcat("LT", output, 128);
    if(nodes[index].op == OP_GT)   my_strcat("GT", output, 128);
    if(nodes[index].op == OP_LE)   my_strcat("LE", output, 128);
    if(nodes[index].op == OP_GE)   my_strcat("GE", output, 128);
    if(nodes[index].op == OP_EQ)   my_strcat("EQ", output, 128);
    if(nodes[index].op == OP_NE)   my_strcat("NE", output, 128);
    if(nodes[index].op == OP_AND)   my_strcat("AND", output, 128);
    if(nodes[index].op == OP_OR)   my_strcat("OR", output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].left, nest+1);
    print_main(nodes[index].right, nest+1);
}

void print_unary(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("UNA(", output, 128);
    if(nodes[index].op == OP_NEG)   my_strcat("NEG", output, 128);
    if(nodes[index].op == OP_NOT)   my_strcat("NOT", output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].left, nest+1);
}

void print_int(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("INT(", output, 128);
    char tmp[12];
    my_int2str(nodes[index].left, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;
}

void print_vardec(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    char tmp[12];
    if(nodes[index].third){
        my_strcat("ARRAYDEC[", output, 128);
        my_int2str(nodes[index].fourth, tmp, 12);
        my_strcat(tmp, output, 128);
        my_strcat("](", output, 128);
    }else{
        my_strcat("VARDEC(", output, 128);
    }
    my_int2str(nodes[index].left, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].right, nest+1);

    print_main(nodes[index].next, nest);
}

void print_varset(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("VARSET", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].left, nest+1);
    print_main(nodes[index].right, nest+1);

    print_main(nodes[index].next, nest);
}

void print_varref(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("VAR(", output, 128);
    char tmp[12];
    my_int2str(nodes[index].left, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;
}

void print_index(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("INDEX", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].left, nest+1);  //変数
    print_main(nodes[index].right, nest+1); //インデックス
}

void print_if(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("IF", output, 128);
    std::cout << output << std::endl;
    
    print_main(nodes[index].left, nest+1);
    print_main(nodes[index].right, nest+1);
    print_main(nodes[index].third, nest+1);

    print_main(nodes[index].next, nest);
}

void print_while(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("WHILE", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].left, nest+1);
    print_main(nodes[index].right, nest+1);

    print_main(nodes[index].next, nest);
}

void print_for(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("FOR", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].left, nest+1);
    print_main(nodes[index].right, nest+1);
    print_main(nodes[index].third, nest+1);
    print_main(nodes[index].fourth, nest+1);

    print_main(nodes[index].next, nest);
}


void print_call(int index, int nest){
    
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("CALL(", output, 128);
    char tmp[12];
    my_int2str(nodes[index].left, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    int i1 = nodes[index].right;
    for(int i=0; i<nodes[index].third; i++){
        print_main(i1, nest+1);
        i1 = nodes[i1].next;
    }

    print_main(nodes[index].next, nest);
}

void print_break(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("BREAK", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].next, nest);
}

void print_return(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output ,128);
    }
    my_strcat("RETURN", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].left, nest+1);
    print_main(nodes[index].next, nest);
}





struct VarEntry{
    int name_id;
    int segment;    //0:argument, 1:local / 0:static
    int index;
};

VarEntry var_table[512];
int var_table_last = 0;
int local_index = 0;
int local_index_tmp = 0;
int argument_index = 0;
int label_count = 0;

int static_table[512];
int static_table_last = 0;


void print_var_table(){
    std::cout << std::endl;
    std::cout << "var table : " << std::endl;
    for(int i=0; i<var_table_last; i++){
        std::cout << var_table[i].name_id << ":" << var_table[i].segment << ":" << var_table[i].index << std::endl;
    }
}

int ref_var_table(int name_id){
    for(int i=0; i<var_table_last; i++){
        if(var_table[i].name_id == name_id){
            return i;
        }
    }
    return -1;
}

int ref_static_table(int name_id){
    for(int i=0; i<static_table_last; i++){
        if(static_table[i] == name_id){
            return i;
        }
    }
    return -1;
}


void compile_expr(int index, char* dst, int dst_size);
void compile_statement(int index, char* dst, int dst_size);
void compile_statement_list(int index, char* dst, int dst_size);



void compile_int(int index, char* dst, int dst_size){
    char tmp[12] = "";
    my_strcat("push const ", dst, dst_size);
    my_int2str(nodes[index].left, tmp, 12);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);
}

void compile_varref(int index, char* dst, int dst_size){
    char tmp[12];
    int i1 = ref_var_table( nodes[index].left );
    if(i1 != -1){
        if(var_table[i1].segment == 0)  my_strcat("push argument ", dst, dst_size);
        else    my_strcat("push local ", dst, dst_size);
        my_int2str(var_table[i1].index, tmp, 12);
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);
    }else{
        i1 = ref_static_table( nodes[index].left);
        my_strcat("push static ", dst, dst_size);
        my_int2str(i1, tmp, 12);
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);
    }
    
}

void compile_index(int index, char* dst, int dst_size){
    char tmp[12];

    //ベースアドレス
    int i1 = ref_var_table( nodes[ nodes[index].left ].left );
    if(i1 != -1){
        if(var_table[i1].segment == 0)  my_strcat("push argument ", dst, dst_size);
        else    my_strcat("push local ", dst, dst_size);
        my_int2str(var_table[i1].index, tmp, 12);
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);
    }else{
        i1 = ref_static_table( nodes[ nodes[index].left ].left );
        my_strcat("push static ", dst, dst_size);
        my_int2str(i1, tmp, 12);
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);
    }

    //インデックス
    compile_expr(nodes[index].right, dst, dst_size);

    my_strcat("push const 1\nadd\nadd\npop pointer 1\n", dst, dst_size);
    my_strcat("push that 0\n", dst, dst_size);

}

void compile_binary(int index, char* dst, int dst_size){
    compile_expr(nodes[index].left, dst, dst_size);
    compile_expr(nodes[index].right, dst, dst_size);
    if(nodes[index].op == OP_ADD)   my_strcat("add\n", dst, dst_size);
    if(nodes[index].op == OP_SUB)   my_strcat("sub\n", dst, dst_size);
    if(nodes[index].op == OP_MUL)   my_strcat("mul\n", dst, dst_size);
    if(nodes[index].op == OP_DIV)   my_strcat("div\n", dst, dst_size);
    if(nodes[index].op == OP_MOD)   my_strcat("mod\n", dst, dst_size);
    if(nodes[index].op == OP_LT)   my_strcat("lt\n", dst, dst_size);
    if(nodes[index].op == OP_GT)   my_strcat("gt\n", dst, dst_size);
    if(nodes[index].op == OP_LE)   my_strcat("gt\nnot\n", dst, dst_size);
    if(nodes[index].op == OP_GE)   my_strcat("lt\nnot\n", dst, dst_size);
    if(nodes[index].op == OP_EQ)   my_strcat("eq\n", dst, dst_size);
    if(nodes[index].op == OP_NE)   my_strcat("eq\nnot\n", dst, dst_size);
    if(nodes[index].op == OP_AND)   my_strcat("and\n", dst, dst_size);
    if(nodes[index].op == OP_OR)   my_strcat("or\n", dst, dst_size);
}

void compile_unary(int index, char* dst, int dst_size){
    compile_expr( nodes[index].left, dst, dst_size );
    if(nodes[index].op == OP_NEG)   my_strcat("neg\n", dst, dst_size);
    if(nodes[index].op == OP_NOT)   my_strcat("not\n", dst, dst_size);
}

void compile_call(int index, char* dst, int dst_size){
    int arg_index = nodes[index].right;

    //引数のセット
    for(int i=0; i<nodes[index].third; i++){
        compile_expr( arg_index, dst, dst_size );
        arg_index = nodes[arg_index].next;
    }

    my_strcat("call ", dst, dst_size);
    my_strcat( name_table[nodes[index].left], dst, dst_size);
    my_strcat(" ", dst, dst_size);
    char tmp[12];
    my_int2str( nodes[index].third, tmp, 12);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);
}

void compile_expr(int index, char* dst, int dst_size){
    if(index == -1) return;
    if(nodes[index].type == NODE_BINARY)    compile_binary(index, dst, dst_size);
    if(nodes[index].type == NODE_UNARY)    compile_unary(index, dst, dst_size);
    if(nodes[index].type == NODE_INT)   compile_int(index, dst, dst_size);
    if(nodes[index].type == NODE_VARREF)    compile_varref(index, dst, dst_size);
    if(nodes[index].type == NODE_INDEX) compile_index(index, dst, dst_size);
    if(nodes[index].type == NODE_CALL)  compile_call(index, dst, dst_size);
    return;
}


void compile_vardec(int index, char* dst, int dst_size){

    char tmp[12];
    char tmp1[12];

    //変数テーブルに登録
    var_table[var_table_last].name_id = nodes[index].left;
    var_table[var_table_last].segment = 1;
    var_table[var_table_last].index = local_index;
    var_table_last++;

    //配列
    if( nodes[index].third ){
        //配列サイズ
        my_strcat("push const ", dst, dst_size);
        my_int2str( nodes[index].fourth, tmp, 12 );
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);

        //アドレス取得
        my_strcat("push const 1\nadd\ncall $alloc 1\n", dst, dst_size);

        my_strcat("push temp 0\npop temp 0\n", dst, dst_size);

        //アドレス代入
        my_strcat("pop local ", dst, dst_size);
        
        my_int2str(local_index, tmp1, 12);
        my_strcat(tmp1, dst, dst_size);
        my_strcat("\n", dst, dst_size);


        //サイズ代入
        my_strcat("push temp 0\npop pointer 1\n", dst, dst_size);

        my_strcat("push const ", dst, dst_size);    //配列サイズ
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);

        my_strcat("pop that 0\n", dst, dst_size);

    }

    //初期値
    if( nodes[index].right != -1 ){

        //初期値セット
        compile_expr( nodes[index].right, dst, dst_size );

        //代入
        my_strcat("pop local ", dst, dst_size);
            
        my_int2str(local_index, tmp, 12);

        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);

    }

    local_index++;
}

void compile_vardec_static(int index, char* dst, int dst_size){

    char tmp[12];
    char tmp1[12];

    //変数テーブルに登録
    static_table[static_table_last] = nodes[index].left;

    my_strcat("\n", dst, dst_size);

    //配列
    if( nodes[index].third ){
        //配列サイズ
        my_strcat("push const ", dst, dst_size);
        my_int2str( nodes[index].fourth, tmp, 12 );
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);

        //アドレス取得
        my_strcat("push const 1\nadd\ncall $alloc 1\n", dst, dst_size);

        my_strcat("push temp 0\npop temp 0\n", dst, dst_size);

        //アドレス代入
        my_strcat("pop static ", dst, dst_size);
        
        my_int2str(static_table_last, tmp1, 12);
        my_strcat(tmp1, dst, dst_size);
        my_strcat("\n", dst, dst_size);

        //サイズ代入
        my_strcat("push temp 0\npop pointer 1\n", dst, dst_size);

        my_strcat("push const ", dst, dst_size);    //配列サイズ
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);

        my_strcat("pop that 0\n", dst, dst_size);

    }

    //初期値
    if( nodes[index].right != -1 ){

        //初期値セット
        compile_expr( nodes[index].right, dst, dst_size );

        //代入
        my_strcat("pop static ", dst, dst_size);
        
        my_int2str(static_table_last, tmp, 12);
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);

    }

    static_table_last++;
}

void compile_varset(int index, char* dst, int dst_size){

    //代入する値
    compile_expr( nodes[index].right, dst, dst_size);

    //代入
    if( nodes[nodes[index].left].type == NODE_VARREF ){
        char tmp[12];
        int i1 = ref_var_table( nodes[nodes[index].left].left );
        if(i1!=-1){
            if(var_table[i1].segment == 0)  my_strcat("pop argument ", dst, dst_size);
            else    my_strcat("pop local ", dst, dst_size);
            my_int2str(var_table[i1].index, tmp, 12);
            my_strcat(tmp, dst, dst_size);
            my_strcat("\n", dst, dst_size);
        }else{
            i1 = ref_static_table( nodes[nodes[index].left].left );
            my_strcat("pop static ", dst, dst_size);
            my_int2str(i1, tmp, 12);
            my_strcat(tmp, dst, dst_size);
            my_strcat("\n", dst, dst_size);
        }
    }else{
        //ベースアドレス
        char tmp[12];
        int i1 = ref_var_table( nodes[ nodes[ nodes[index].left ].left ].left );
        if(i1!=-1){
            if(var_table[i1].segment == 0)  my_strcat("push argument ", dst, dst_size);
            else    my_strcat("push local ", dst, dst_size);
            my_int2str(var_table[i1].index, tmp, 12);
            my_strcat(tmp, dst, dst_size);
            my_strcat("\n", dst, dst_size);
        }else{
            i1 = ref_static_table( nodes[ nodes[ nodes[index].left ].left ].left );
            my_strcat("push static ", dst, dst_size);
            my_int2str(i1, tmp, 12);
            my_strcat(tmp, dst, dst_size);
            my_strcat("\n", dst, dst_size);
        }

        //インデックス
        compile_expr(nodes[ nodes[index].left ].right, dst, dst_size);

        my_strcat("push const 1\nadd\nadd\npop pointer 1\n", dst, dst_size);

        //代入
        my_strcat("pop that 0\n", dst, dst_size);

    }
}


void compile_if(int index, char* dst, int dst_size){

    char tmp[12] = "";
    my_int2str(++label_count, tmp, 12);

    //判定式
    compile_expr( nodes[index].left, dst, dst_size );

    my_strcat("ifgo ", dst, dst_size);
    my_strcat("if_then_", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    //else
    compile_statement_list( nodes[index].third, dst, dst_size );

    my_strcat("goto ", dst, dst_size);
    my_strcat("if_end_", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    my_strcat("label ", dst, dst_size);
    my_strcat("if_then_", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    //then
    compile_statement_list( nodes[index].right, dst, dst_size );

    my_strcat("label ", dst, dst_size);
    my_strcat("if_end_", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

}

void compile_while(int index, char* dst, int dst_size){
    char tmp[12] = "";
    my_int2str(++label_count, tmp, 12);

    my_strcat("label loopst", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    //判定式
    compile_expr( nodes[index].left, dst, dst_size);

    my_strcat("not\nifgo loopen", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    //処理
    compile_statement_list( nodes[index].right, dst, dst_size );
    
    my_strcat("goto loopst", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    my_strcat("label loopen", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

}

void compile_for(int index, char* dst, int dst_size){
    local_index_tmp = local_index;
    char tmp[12] = "";
    my_int2str(++label_count, tmp, 12);

    //ローカル変数定義
    compile_vardec( nodes[index].left, dst, dst_size);

    my_strcat("label forst", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    //判定式
    compile_expr( nodes[index].right, dst, dst_size);

    my_strcat("not\nifgo loopen", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    //処理
    compile_statement_list( nodes[index].fourth, dst, dst_size);

    //末尾の処理
    compile_statement( nodes[index].third, dst, dst_size );

    my_strcat("goto forst", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    my_strcat("label loopen", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    local_index = local_index_tmp;
}

void compile_return(int index, char* dst, int dst_size){
    //返り値
    compile_expr( nodes[index].left, dst, dst_size);
    //リターン
    my_strcat("return\n", dst, dst_size);
}

void compile_break(int index, char* dst, int dst_size){
    char tmp[12] = "";
    my_int2str(label_count, tmp, 12);
    my_strcat("goto loopen", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);
}

void compile_procedure(int index, char* dst, int dst_size){
    compile_call(index, dst, dst_size);
    my_strcat("pop temp 0\n", dst, dst_size);
}

void compile_statement(int index, char* dst, int dst_size){
    if(nodes[index].type == NODE_VARDEC)    compile_vardec(index, dst, dst_size);
    if(nodes[index].type == NODE_VARSET)   compile_varset(index, dst, dst_size);
    if(nodes[index].type == NODE_IF)    compile_if(index, dst, dst_size);
    if(nodes[index].type == NODE_WHILE)    compile_while(index, dst, dst_size);
    if(nodes[index].type == NODE_FOR)    compile_for(index, dst, dst_size);
    if(nodes[index].type == NODE_RETURN)    compile_return(index, dst, dst_size);
    if(nodes[index].type == NODE_BREAK)    compile_break(index, dst, dst_size);
    if(nodes[index].type == NODE_CALL)    compile_procedure(index, dst, dst_size);
}

void compile_statement_list(int index, char* dst, int dst_size){
    local_index_tmp = local_index;
    while(index != -1){
        compile_statement(index, dst, dst_size);
        index = nodes[index].next;
    }
    local_index = local_index_tmp;
}


void compile_func(int index, char* dst, int dst_size){

    char tmp[64] = "";

    var_table_last = 0;
    argument_index = 0;
    local_index = 0;
    local_index_tmp = 0;

    //関数宣言
    my_strcat("\nfunction ", dst, dst_size);
    my_strcat(name_table[nodes[index].left], dst, dst_size);
    my_strcat(" ", dst, dst_size);
    my_int2str(nodes[index].local_count, tmp, 64);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    //引数セット
    int arg_index = nodes[index].third;
    for(int i=0; i<nodes[index].fourth; i++){
        var_table[var_table_last].name_id = nodes[arg_index].left;
        var_table[var_table_last].segment = 0;
        var_table[var_table_last].index = argument_index++;

        var_table_last++;
        arg_index = nodes[arg_index].next;
    }

    //文
    int statement_index = nodes[index].body;
    compile_statement_list( statement_index, dst, dst_size );
}

void compile_main(int index, char* dst, int dst_size){
    while(index != -1){
        if(nodes[index].type == NODE_FUNC)  compile_func(index, dst, dst_size);
        index = nodes[index].next;
    }
}

void compile_static(int index, char* dst, int dst_size){
    while(index != -1){
        if(nodes[index].type == NODE_VARDEC)  compile_vardec_static(index, dst, dst_size);
        index = nodes[index].next;
    }
}





VM vm;


int main(){

    my_strcpy(jack_script0, jack_script, 1000);

    //コメント除去
    remove_comment();

    //トークナイズ
    char output[2048] = "";

    tokenize_index = 0;

    std::cout << "\ntokens :" << std::endl;
    char tmp[128];
    while(tokenize_index < my_strlen(jack_script)-1){
        nextToken(tmp, 128, true);
        my_strcat(tmp, output, 2048);
        my_strcat(" ", output, 2048);
    }
    std::cout << output << std::endl;

    tokenize_index = 0;
    output[0] = '\0';


    int root_node = parse_main();

    std::cout << "\nnodes :" << std::endl;
    print_main(root_node, 0);

    std::cout << "\nname table :" << std::endl;
    for(int i=0; i<name_table_index; i++){
        std::cout << i << " : " << name_table[i] << std::endl;
    }


    
    
    if(error_line != -1){
        std::cout << "\nsyntax error : " << std::endl;
        std::cout << "line : " << error_line << std::endl;
        source_of_line(error_line, output, 2048);
        std::cout << output << std::endl;
    }else{
        //コンパイル
        output[0] = '\0';
        compile_static(root_node, output, 2048);
        my_strcat(vm_header, output, 2048);
        compile_main(root_node, output, 2048);
        my_strcat(vm_footer, output, 2048);

        std::cout << std::endl;
        std::cout << "vm script : " << std::endl;
        std::cout << output << std::endl;

        //実行
        vm.run(output, output, 2048);
        std::cout << "\noutput : \n" << output << std::endl;

        std::cout << "### stack ###" << std::endl;
        char s1[] = " : ";
        for(int i=0; i<40; i++){
            if(i==vm.sp) s1[1] = '#';
            else    s1[1] = ':';
            std::cout << i << s1 << vm.stack[i] << std::endl;
        }

        std::cout << std::endl;
        std::cout << "### heap ###" << std::endl;
        for(int i=0; i<20; i++){
            std::cout << i << " : " << vm.heap[i] << std::endl;
        }

        std::cout << std::endl;
        std::cout << "### static ###" << std::endl;
        for(int i=0; i<20; i++){
            std::cout << i << " : " << vm.statics[i] << std::endl;
        }

    }



    // compile_main(root_node, output, 512);
    // my_strcat("out\n", output, 512);
    
    
    // vm.run(output, output ,128);
    // std::cout << "\noutput : \n" << output << std::endl;
    





    return 0;
}