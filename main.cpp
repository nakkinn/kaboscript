//実行コマンド　g++ main.cpp string_utils.cpp virtual_machine.cpp -o main && main

#include <iostream>

#include "string_utils.hpp"
#include "virtual_machine.hpp"

const char* vm_header = R"(
call Main.main 0
pop temp 0
goto $end
)";

const char* vm_footer = R"(
function Output.printi 0
push argument 0
out
push const 0
return

function Output.printc 0
push argument 0
outc
push const 0
return

function Memory.alloc 0
push argument 0
call $alloc 1
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

class Calc {                                                                                  int val;                                                                            
    static int count;                                                                                                                                                             
    constructor new(int v) {                                                            
        val = v;                                                                        
        count++;
    }

    method add(int n) {
        val += n;
        return val;
    }

    method get() { return val; }

    function getCount() { return count; }
}

class Main {
    function main() {

        // 算術・比較
        Output.printi(3 + 4);    Output.printc(' ');  // 7
        Output.printi(10 - 3);   Output.printc(' ');  // 7
        Output.printi(3 * 4);    Output.printc(' ');  // 12
        Output.printi(10 > 3);   Output.printc(' ');  // 1
        Output.printi(10 == 9);  Output.printc(' ');  // 0

        // ローカル変数・制御構文
        int sum = 0;
        for(int i = 1; i <= 5; i++) {
            sum += i;
        }
        Output.printi(sum);  Output.printc(' ');  // 15

        // 配列
        int arr[3];
        arr[0] = 10; arr[1] = 20; arr[2] = 30;
        Output.printi(arr[1]);  Output.printc(' ');  // 20

        // constructor + method + static
        Calc c1;
        Calc c2;
        c1 = Calc.new(10);
        c2 = Calc.new(20);
        c1.add(5);
        Output.printi(c1.get());          Output.printc(' ');  // 15
        Output.printi(c2.get());          Output.printc(' ');  // 20
        Output.printi(Calc.getCount());   Output.printc(' ');  // 2

        return 0;
    }
}



)";

char jack_script[100000];




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
            error_line = line_of_token(tokenize_index);
        }
    }
}





enum NodeType{
    NODE_CLASS,     //left:name_id, right:field_count, body, next
    NODE_FUNC,      //left:name_id, right:return_type, third:arg_first, fourth:arg_count, local_count, kind, body, next
    NODE_ARG,       //left:var_id, right:type, next
    NODE_INT,       //left:value
    NODE_VARREF,
    NODE_INDEX,     //left:var_id, right:index, next
    NODE_BINARY,
    NODE_UNARY,
    NODE_VARDEC,    //left:var_id, right:type, third:is_static, fourth:array_size, body:init_expr, next
    NODE_VARSET,    //left:var_id, right:expr, next
    NODE_IF,        //left:condition, right:then_head_id, third:else_head_id, next
    NODE_WHILE,     //left:condition, right:then_head_id, next
    NODE_FOR,       //left:init, right:cond, third:update, foruth:statement, next
    NODE_CALL,      //left:func_id, right:type, third:arg_head_id, fourth:arg_count, body:instance, next
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

enum FuncKind{
    FUNC,
    METHOD,
    CONSTRUCTOR
};

enum VarTable{
    TABLE_FUNC,
    TABLE_CLASS
};

enum FuncSeg{
    SEG_ARG,
    SEG_LOCAL
};

enum ClassSeg{
    SEG_FIELD,
    SEG_STATIC
};

struct NameEntry{
    char name[64];
    bool is_type;
};

struct IntNode{int value;};
struct UnaryNode{int op; int left;};
struct BinaryNode{int op; int left; int right;};
struct VarRefNode{int id;};
struct IndexNode{int id; int index;};
struct ArgNode{int id; int type;};
struct VarDecNode{int id; int type; int is_static; int array_size; int expr;};
struct VarSetNode{int target; int expr;};
struct IfNode{int cond; int then_; int else_;};
struct WhileNode{int cond; int then_;};
struct ForNode{int init; int cond; int update; int body;};
struct CallNode{int func_id; int type; int arg_first; int arg_count; int instance;};
struct BreakNode{};
struct ReturnNode{int expr;};
struct FuncNode{int id; int kind; int arg_first; int arg_count; int local_count; int body;};
struct ClassNode{int id; int field_count; int body;};

struct Node{
    NodeType type;
    int next;
    int token;
    union{
        IntNode int_;
        UnaryNode unary;
        BinaryNode binary;
        VarRefNode varref;
        IndexNode index;
        ArgNode arg;
        VarDecNode vardec;
        VarSetNode varset;
        IfNode if_;
        WhileNode while_;
        ForNode for_;
        CallNode call;
        BreakNode break_;
        ReturnNode return_;
        FuncNode func;
        ClassNode class_;
    };
};

struct VarEntry{
    int name_id;
    int type;
    int segment;    //0:argument, 1:local / 0:field, 1:static
    int index;
};

struct VarLookup{
    int table;
    int index;
};



static const int node_max = 512;
int node_index = 0;
Node nodes[node_max];

int name_table_max = 512;
int name_table_index = 0;
NameEntry name_table[512];

int local_count, current_local;
int field_count;




void initialize_name_table(){
    for(int i=0; i<name_table_max; i++){
        name_table[i].name[0] = '\0';
    }
    name_table_index = 0;
}

int set_name_table(const char* src, bool is_type){

    //既に登録済み
    for(int i=0; i<name_table_index; i++){
        if(my_streq(src, name_table[i].name))    return i;
    }

    //新規登録
    if( my_streq(src, "int"))   name_table[name_table_index].is_type = true;
    if( is_type )   name_table[name_table_index].is_type = true;

    my_strcpy( src, name_table[name_table_index++].name, 64);
    return name_table_index - 1;
}

int ref_name_table(const char* src){
    for(int i=0; i<name_table_index; i++){
        if(my_streq(src, name_table[i].name))    return i;
    }
    return -1;
}

bool istype_name_table(const char* src){
    for(int i=0; i<name_table_index; i++){
        if(my_streq(src, name_table[i].name))   return name_table[i].is_type;
    }
    return false;
}

//intとクラスの名前をname_tableにセット
void get_type_name(){
    //初期化
    name_table_index = 0;
    for(int i=0; i<name_table_max; i++){
        name_table[i].name[0] = '\0';
        name_table[i].is_type = false;
    }

    my_strcpy( "int", name_table[name_table_index].name, 64);
    name_table[name_table_index++].is_type = true;

    my_strcpy("Output", name_table[name_table_index].name, 64);
    name_table[name_table_index++].is_type = true;

    my_strcpy("Memory", name_table[name_table_index].name, 64);
    name_table[name_table_index++].is_type = true;

    tokenize_index = 0;
    char tmp[64];
    nextToken(tmp, 64, true);
    while(tmp[0]!='\0'){
        if(my_streq(tmp, "class")){
            nextToken(tmp, 64, true);
            set_name_table(tmp, true);
        }
        nextToken(tmp, 64, true);
    }
    tokenize_index = 0;
}



VarEntry func_var_table[512];
int func_table_last = 0;
int local_index = 0;
int local_index_tmp = 0;
int argument_index = 0;

VarEntry class_var_table[512];
int class_table_last = 0;
int field_index = 0;
int static_index = 0;

int label_count = 0;
int current_class_id = -1;


int ref_func_var(int name_id){
    for(int i=0; i<func_table_last; i++){
        if(func_var_table[i].name_id == name_id){
            return i;
        }
    }
    return -1;
}

int ref_class_var(int name_id){
    for(int i=0; i<class_table_last; i++){
        if(class_var_table[i].name_id == name_id){
            return i;
        }
    }
    return -1;
}

//x: (0:func, 1:class), y:index
VarLookup ref_var(int name_id){
    VarLookup result;
    result.table = 0;
    result.index = ref_func_var(name_id);
    if(result.index != -1)  return result;

    result.table = 1;
    result.index = ref_class_var(name_id);

    if(result.index==-1){
        std::cout << "Error : " << name_table[name_id].name << " is not defined." << std::endl;
    }

    return result;
}


int ref_type(int name_id){
    for(int i=0; i<func_table_last; i++){
        if(func_var_table[i].name_id == name_id)    return func_var_table[i].type;
    }
    for(int i=0; i<class_table_last; i++){
        if(class_var_table[i].name_id == name_id)   return class_var_table[i].type;
    }
    return -1;
}




//##################################################################
//　　ノード生成
//##################################################################

int new_int(int value){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_INT;
    nodes[idx].int_.value = value;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_unary(int op, int left){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_UNARY;
    nodes[idx].unary.op = op;
    nodes[idx].unary.left = left;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_binary(int op, int left, int right){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_BINARY;
    nodes[idx].binary.op = op;
    nodes[idx].binary.left = left;
    nodes[idx].binary.right = right;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_varref(int id){
    int idx = node_index++;
    nodes[idx].type = NODE_VARREF;
    nodes[idx].varref.id = id;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_index(int varid, int index){
    int idx = node_index++;
    nodes[idx].type = NODE_INDEX;
    nodes[idx].index.id = varid;
    nodes[idx].index.index = index;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_argdec(int id, int type){
    int idx = node_index++;
    nodes[idx].type = NODE_ARG;
    nodes[idx].arg.id = id;
    nodes[idx].arg.type = type;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_vardec(int id, int type, int is_static, int array_size, int init){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_VARDEC;
    nodes[idx].vardec.id = id;
    nodes[idx].vardec.type = type;
    nodes[idx].vardec.is_static = is_static;
    nodes[idx].vardec.array_size = array_size;
    nodes[idx].vardec.expr = init;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_varset(int id, int expr){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_VARSET;
    nodes[idx].varset.target = id;
    nodes[idx].varset.expr = expr;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_if(int cond, int then_index, int else_index){
    int idx = node_index++;
    nodes[idx].type = NODE_IF;
    nodes[idx].if_.cond = cond;
    nodes[idx].if_.then_ = then_index;
    nodes[idx].if_.else_ = else_index;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_while(int cond, int then_index){
    int idx = node_index++;
    nodes[idx].type = NODE_WHILE;
    nodes[idx].while_.cond = cond;
    nodes[idx].while_.then_ = then_index;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_for(int init, int cond, int update, int stmt){
    int idx = node_index++;
    nodes[idx].type = NODE_FOR;
    nodes[idx].for_.init = init;
    nodes[idx].for_.cond = cond;
    nodes[idx].for_.update = update;
    nodes[idx].for_.body = stmt;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_call(int id, int type, int arg_index, int arg_count, int instance){
    int idx = node_index++;
    nodes[idx].type = NODE_CALL;
    nodes[idx].call.func_id = id;
    nodes[idx].call.type = type;
    nodes[idx].call.arg_first = arg_index;
    nodes[idx].call.arg_count = arg_count;
    nodes[idx].call.instance = instance;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_break(){
    int idx = node_index++;
    nodes[idx].type = NODE_BREAK;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_return(int expr){
    int idx = node_index++;
    nodes[idx].type = NODE_RETURN;
    nodes[idx].return_.expr = expr;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_func(int id, int kind, int arg_first, int arg_count, int local_count, int body){
    int idx = node_index++;
    nodes[idx].type = NODE_FUNC;
    nodes[idx].func.kind = kind;
    nodes[idx].func.id = id;
    nodes[idx].func.arg_first = arg_first;
    nodes[idx].func.arg_count = arg_count;
    nodes[idx].func.local_count = local_count;
    nodes[idx].func.body = body;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}

int new_class(int id, int field_count, int body){
    int idx = node_index++;
    nodes[idx].type = NODE_CLASS;
    nodes[idx].class_.id = id;
    nodes[idx].class_.field_count = field_count;
    nodes[idx].class_.body = body;
    nodes[idx].next = -1;
    nodes[idx].token = -1;
    return idx;
}



//##################################################################
//　　AST生成
//##################################################################

int parse_main();

int parse_class();
int parse_func();

int parse_statement_list();
int parse_statement();

int parse_class_vardec();
int parse_vardec();
int parse_varset(int target);
int parse_if();
int parse_while();
int parse_for();
int parse_call();
int parse_method(int target);
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
int parse_lvalue();


int parse_main(){
    if(error_line != -1)    return -1;

    int name_id=-1, body_id_origin=-1, body_id0=-1, body_id=-1;
    field_count = 0;

    char tmp[64];
    nextToken(tmp, 64, false);
    while(tmp[0]!='\0'){

        int prev = tokenize_index;

        body_id = parse_class();

        if(prev == tokenize_index)  error_line = line_of_token(tokenize_index);
        if(error_line != -1)    return -1;

        if(body_id_origin == -1)   body_id_origin = body_id;
        if(body_id0 != -1)  nodes[body_id0].next = body_id;
        body_id0 = body_id;

        nextToken(tmp, 64, false);
    }

    return body_id_origin;

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
        int op;
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
        int op;
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
    }else if(tmp1[0]=='\''){    //文字
        expect("'");
        nextToken(tmp1, 64, true);
        if(tmp1[0] == '\''){    //空白
            node = new_int(' ');
        }else if(tmp1[0]=='\\'){
            nextToken(tmp1, 64, true);
            if(tmp1[0] == 'n')  node = new_int('\n');
            if(tmp1[0] == '\\')  node = new_int('\\');
            if(tmp1[0] == '\'') node = new_int('\'');
            expect("'");
        }else{
            if(tmp1[1] != '\0') error_line = line_of_token(tokenize_index);
            node = new_int(tmp1[0]);
            expect("'");
        }
    }else if( tmp1[0] == '(' ){  //式
        expect("(");
        node = parse_expr();
        expect(")");
    }else if( istype_name_table(tmp1) && tmp2[0]=='.'){  //型名（function, constructor）
        node = parse_call();
    }else if( is_ident_head(tmp1[0]) ){  //変数, 配列, method
        node = parse_lvalue();
        nextToken(tmp1, 64, false);
        if(tmp1[0] == '.'){ //method
            node = parse_method(node);
        }
    }else{
        error_line = line_of_token(tokenize_index);
    }
    
    return node;
}

int parse_lvalue(){

    if(error_line != -1)    return -1;
    char tmp1[64];
    char tmp2[64];
    nextToken(tmp1, 64, false);
    nnextToken(tmp2, 64);
    int node = -1;

    nextToken(tmp1, 64, true);
    node = new_varref( set_name_table(tmp1, false) );  //ref_name_table 
    if(tmp2[0] == '['){
        expect("[");
        node = new_index(node, parse_expr());
        expect("]");
    }
    return node;
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
    
    if( istype_name_table(tmp1) && tmp2[0]!='.' ){
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
    }else if( istype_name_table(tmp1) && tmp2[0]=='.' ){
        node = parse_call();    
        expect(";");
    }else{
        node = parse_lvalue();
        nextToken(tmp1, 64, false);
        if(tmp1[0]=='.')    node = parse_method(node);
        else    node = parse_varset(node);
        expect(";");
    } 

    return node;
}

int parse_class_vardec(){
    if(error_line != -1)    return -1;
    char tmp[64];
    int id=-1, type=-1, is_static=0, array_size=-1;
    
    nextToken(tmp, 64, false);
    if(my_streq(tmp,"static")){
        is_static = 1;
        expect("static");
    }else{
        field_count++;
    }

    nextToken(tmp, 64, true);   //型名
    type = set_name_table(tmp, false);

    nextToken(tmp, 64, true);   //変数名
    id = set_name_table(tmp, false);

    nextToken(tmp, 64, false);  // [ を確認
    if( tmp[0] == '[' ){
        expect("[");
        nextToken(tmp, 64, true);   //配列サイズ（整数）
        array_size = my_str2int(tmp);
        expect("]");
    }

    return new_vardec(id, type, is_static, array_size, -1);
}

int parse_vardec(){
    if(error_line != -1)    return -1;
    char tmp[64];
    int id;
    int init = -1;
    int type = -1;
    int array_size = -1;

    current_local++;
    if(current_local > local_count) local_count = current_local;

    nextToken(tmp, 64, true);  //型名
    type = set_name_table(tmp, false);

    nextToken(tmp, 64, true);  //変数名
    id = set_name_table(tmp, false);

    nextToken(tmp, 64, false);  // [ 確認
    if(tmp[0] == '['){
        expect("[");
        nextToken(tmp, 64, false);
        if(tmp[0] != ']'){
            nextToken(tmp, 64, true);   //サイズ
            array_size = my_str2int(tmp);
        }else{
            array_size = 0;
        }
        expect("]");
    }

    nextToken(tmp, 64, false);  // = 確認
    if(tmp[0]=='='){ 
        expect("=");
        if(array_size==-1)  init = parse_expr();
        else{
            int element=-1, element0=-1;
            array_size = 0;
            expect("{");
            nextToken(tmp, 64, false);
            while(tmp[0] != '}'){
                if(array_size>0)    expect(",");
                element = parse_expr();
                if(init==-1)    init = element;
                if(element0!=-1)    nodes[element0].next = element;
                element0 = element;
                array_size++;
                nextToken(tmp, 64, false);
            }
            expect("}");
        }
    }
    return new_vardec(id, type, 0, array_size, init);
}

int parse_varset(int target){
    if(error_line != -1)    return -1;
    char tmp[64];
    int id = target;
    int value;

    // id = parse_expr();
    
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

    std::cout << "debug:for" << std::endl;

    expect("for");
    expect("(");

    nextToken(tmp, 64, false);  //確認
    if(tmp[0]!=';') init = parse_vardec(); 
    expect(";");

    nextToken(tmp, 64, false);  //確認
    if(tmp[0]!=';') cond = parse_expr();
    expect(";");

    nextToken(tmp, 64, false);  //確認
    if(tmp[0]!=')'){
        update = parse_varset(parse_lvalue());
    }
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
    int type = -1;

    nextToken(tmp, 64, true);   //クラス名
    type = ref_name_table(tmp);

    expect(".");

    nextToken(tmp, 64, true);
    id = set_name_table(tmp, false);   //関数名
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

    nextToken(tmp, 64, false);

    return new_call(id, type, arg_origin, arg_count, -1);
}

int parse_method(int instance){
    if(error_line != -1)    return -1;
    char tmp[64];
    int id = -1;
    int arg_index0 = -1;
    int arg_index1 = -1;
    int arg_origin = -1;
    int arg_count = 0;
    int type = -1;

    expect(".");

    nextToken(tmp, 64, true);
    id = set_name_table(tmp, false);   //関数名
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

    nextToken(tmp, 64, false);

    return new_call(id, type, arg_origin, arg_count, instance);
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


int parse_func(){
    if(error_line != -1)    return -1;
    char tmp[64];
    int func_id, body_id, kind=-1;
    int arg_id=-1, arg_id0=-1, arg_origin=-1, arg_count=0, arg_type;

    current_local = 0;
    local_count = 0;
    
    nextToken(tmp, 64, true);   //function:0 or method:1 or constructor:2
    if(my_streq(tmp, "function"))   kind = FUNC;
    if(my_streq(tmp, "method")) kind = METHOD;
    if(my_streq(tmp, "constructor"))    kind = CONSTRUCTOR;
    if(kind == -1){
        error_line = line_of_token(tokenize_index);
        return -1;
    }

    // nextToken(tmp, 64, true);   //返り値の型
    // return_type = set_name_table(tmp, false);

    nextToken(tmp, 64, true);   //関数名
    func_id = set_name_table(tmp, false);

    expect("(");

    nextToken(tmp, 64, false);

    while(tmp[0]!='\0' && tmp[0]!=')'){
        if(arg_origin!=-1)  expect(",");
        nextToken(tmp, 64, true);
        arg_type = set_name_table(tmp, false);
        nextToken(tmp, 64, true);
        arg_id = new_argdec( set_name_table(tmp, false), arg_type );
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
    
    return new_func(func_id, kind, arg_origin, arg_count, local_count, body_id);
}

int parse_class(){
    if(error_line != -1)    return -1;

    int name_id=-1, body_id_origin=-1, body_id0=-1, body_id=-1;
    field_count = 0;

    char tmp[64];

    expect("class");
    
    nextToken(tmp, 64, true);   //クラス名
    name_id = set_name_table(tmp, true);

    expect("{");

    nextToken(tmp, 64, false);
    while(tmp[0]!='\0' && tmp[0]!='}'){

        int prev = tokenize_index;

        if(my_streq(tmp,"function") || my_streq(tmp,"method") || my_streq(tmp,"constructor")){
            body_id = parse_func();
        }else{
            body_id = parse_class_vardec();
            expect(";");
        }

        if(prev == tokenize_index)  error_line = line_of_token(tokenize_index);
        if(error_line != -1)    return -1;

        if(body_id_origin == -1)   body_id_origin = body_id;
        if(body_id0 != -1)  nodes[body_id0].next = body_id;
        body_id0 = body_id;

        nextToken(tmp, 64, false);
    }

    expect("}");
    return new_class(name_id, field_count, body_id_origin);
}



//##################################################################
//　　デバッグ用出力
//##################################################################

void print_main(int index, int nest);
void print_int(int index, int nest);
void print_unary(int index, int nest);
void print_binary(int index, int nest);
void print_varref(int index, int nest);
void print_index(int index, int nest);
void print_argdec(int index, int nest);
void print_vardec(int index, int nest);
void print_varset(int index, int nest);
void print_if(int index, int nest);
void print_while(int index, int nest);
void print_for(int index, int nest);
void print_call(int index, int nest);
void print_break(int index, int nest);
void print_return(int index, int nest);
void print_func(int index, int nest);
void print_class(int index, int nest);


void print_main(int index, int nest){
    if(index == -1) return;
    if(nodes[index].type == NODE_CLASS)  print_class(index, nest);
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

void print_int(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("INT(", output, 128);
    char tmp[12];
    my_int2str(nodes[index].int_.value, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].next, nest);
}

void print_unary(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("UNA(", output, 128);
    if(nodes[index].unary.op == OP_NEG)   my_strcat("NEG", output, 128);
    if(nodes[index].unary.op == OP_NOT)   my_strcat("NOT", output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].unary.left, nest+1);
    print_main(nodes[index].next, nest);
}

void print_binary(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("BIN(", output, 128);
    if(nodes[index].binary.op == OP_ADD)   my_strcat("ADD", output, 128);
    if(nodes[index].binary.op == OP_SUB)   my_strcat("SUB", output, 128);
    if(nodes[index].binary.op == OP_MUL)   my_strcat("MUL", output, 128);
    if(nodes[index].binary.op == OP_DIV)   my_strcat("DIV", output, 128);
    if(nodes[index].binary.op == OP_MOD)   my_strcat("MOD", output, 128);
    if(nodes[index].binary.op == OP_LT)   my_strcat("LT", output, 128);
    if(nodes[index].binary.op == OP_GT)   my_strcat("GT", output, 128);
    if(nodes[index].binary.op == OP_LE)   my_strcat("LE", output, 128);
    if(nodes[index].binary.op == OP_GE)   my_strcat("GE", output, 128);
    if(nodes[index].binary.op == OP_EQ)   my_strcat("EQ", output, 128);
    if(nodes[index].binary.op == OP_NE)   my_strcat("NE", output, 128);
    if(nodes[index].binary.op == OP_AND)   my_strcat("AND", output, 128);
    if(nodes[index].binary.op == OP_OR)   my_strcat("OR", output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].binary.left, nest+1);
    print_main(nodes[index].binary.right, nest+1);
    print_main(nodes[index].next, nest+1);

}

void print_varref(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("VAR(", output, 128);
    char tmp[12];
    my_int2str(nodes[index].varref.id, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].next, nest);
}

void print_index(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("INDEX", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].index.id, nest+1);  //変数
    print_main(nodes[index].index.index, nest+1); //インデックス
    print_main(nodes[index].next, nest);
}

void print_argdec(int index, int nest){
    char output[128] = "";
    char tmp[12];
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("ARG(", output, 128);
    my_int2str(nodes[index].arg.type, tmp, 12);    //型
    my_strcat(tmp, output, 128);
    my_strcat(",", output, 128);
    my_int2str(nodes[index].arg.id, tmp, 12);     //名前
    my_strcat(tmp, output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].next, nest);
}

void print_vardec(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    char tmp[12];
    if(nodes[index].vardec.array_size != -1){
        my_strcat("ARRAYDEC[", output, 128);
        my_int2str(nodes[index].vardec.array_size, tmp, 12);
        my_strcat(tmp, output, 128);
        my_strcat("](", output, 128);
    }else{
        my_strcat("VARDEC(", output, 128);
    }
    my_int2str(nodes[index].vardec.type, tmp, 12); //型id
    my_strcat(tmp, output, 128);
    my_strcat(",", output, 128);
    my_int2str(nodes[index].vardec.id, tmp, 12);    //名前id
    my_strcat(tmp, output, 128);
    my_strcat(",", output, 128);
    my_int2str(nodes[index].vardec.is_static, tmp, 12);     //static
    my_strcat(tmp, output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].vardec.expr, nest+1);  //初期値

    print_main(nodes[index].next, nest);
}

void print_varset(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("VARSET", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].varset.target, nest+1);
    print_main(nodes[index].varset.expr, nest+1);

    print_main(nodes[index].next, nest);
}

void print_if(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("IF", output, 128);
    std::cout << output << std::endl;
    
    print_main(nodes[index].if_.cond, nest+1);
    print_main(nodes[index].if_.then_, nest+1);
    print_main(nodes[index].if_.else_, nest+1);

    print_main(nodes[index].next, nest);
}

void print_while(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("WHILE", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].while_.cond, nest+1);
    print_main(nodes[index].while_.then_, nest+1);

    print_main(nodes[index].next, nest);
}

void print_for(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("FOR", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].for_.init, nest+1);
    print_main(nodes[index].for_.cond, nest+1);
    print_main(nodes[index].for_.update, nest+1);
    print_main(nodes[index].for_.body, nest+1);

    print_main(nodes[index].next, nest);
}

void print_call(int index, int nest){
    
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("CALL(", output, 128);
    char tmp[12];
    my_int2str(nodes[index].call.type, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(",", output, 128);
    my_int2str(nodes[index].call.func_id, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    int i1 = nodes[index].call.arg_first;
    for(int i=0; i<nodes[index].call.arg_count; i++){
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

    print_main(nodes[index].return_.expr, nest+1);
    print_main(nodes[index].next, nest);
}

void print_func(int index, int nest){
    char output[128] = "";
    char tmp[12];
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }

    if(nodes[index].func.kind==FUNC)    my_strcat("FUNC(", output, 128);
    if(nodes[index].func.kind==METHOD)    my_strcat("METHOD(", output, 128);
    if(nodes[index].func.kind==CONSTRUCTOR)    my_strcat("CONSTRUCTOR(", output, 128);


    my_int2str(nodes[index].func.id, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(",", output, 128);
    my_int2str(nodes[index].func.arg_count, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(",", output, 128);
    my_int2str(nodes[index].func.local_count, tmp, 12);
    my_strcat(tmp, output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].func.arg_first, nest+1);
    print_main(nodes[index].func.body, nest+1);

    print_main(nodes[index].next, nest);
}

void print_class(int index, int nest){
    char output[128] = "";
    char tmp[12];
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("CLASS(", output, 128);
    my_int2str(nodes[index].class_.id, tmp, 12); //名前id
    my_strcat(tmp, output, 128);
    my_strcat(",", output, 128);
    my_int2str(nodes[index].class_.field_count, tmp, 12);    //field_count
    my_strcat(tmp, output, 128);
    my_strcat(")", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].class_.body, nest+1);
    print_main(nodes[index].next, nest);
}



//##################################################################
//　　コンパイル
//##################################################################

void compile_expr(int index, char* dst, int dst_size);
void compile_statement(int index, char* dst, int dst_size);
void compile_statement_list(int index, char* dst, int dst_size);
void compile_class(int index, char* dst, int dst_size);


void compile_main(int index, char* dst, int dst_size){

    while(index != -1){
        compile_class(index, dst, dst_size);
        index = nodes[index].next;
    }

}

void compile_int(int index, char* dst, int dst_size){
    char tmp[12] = "";
    my_strcat("push const ", dst, dst_size);
    my_int2str(nodes[index].int_.value, tmp, 12);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);
}

void compile_unary(int index, char* dst, int dst_size){
    compile_expr( nodes[index].unary.left, dst, dst_size );
    if(nodes[index].unary.op == OP_NEG)   my_strcat("neg\n", dst, dst_size);
    if(nodes[index].unary.op == OP_NOT)   my_strcat("not\n", dst, dst_size);
}

void compile_binary(int index, char* dst, int dst_size){
    compile_expr(nodes[index].binary.left, dst, dst_size);
    compile_expr(nodes[index].binary.right, dst, dst_size);
    if(nodes[index].binary.op == OP_ADD)   my_strcat("add\n", dst, dst_size);
    if(nodes[index].binary.op == OP_SUB)   my_strcat("sub\n", dst, dst_size);
    if(nodes[index].binary.op == OP_MUL)   my_strcat("mul\n", dst, dst_size);
    if(nodes[index].binary.op == OP_DIV)   my_strcat("div\n", dst, dst_size);
    if(nodes[index].binary.op == OP_MOD)   my_strcat("mod\n", dst, dst_size);
    if(nodes[index].binary.op == OP_LT)   my_strcat("lt\n", dst, dst_size);
    if(nodes[index].binary.op == OP_GT)   my_strcat("gt\n", dst, dst_size);
    if(nodes[index].binary.op == OP_LE)   my_strcat("gt\nnot\n", dst, dst_size);
    if(nodes[index].binary.op == OP_GE)   my_strcat("lt\nnot\n", dst, dst_size);
    if(nodes[index].binary.op == OP_EQ)   my_strcat("eq\n", dst, dst_size);
    if(nodes[index].binary.op == OP_NE)   my_strcat("eq\nnot\n", dst, dst_size);
    if(nodes[index].binary.op == OP_AND)   my_strcat("and\n", dst, dst_size);
    if(nodes[index].binary.op == OP_OR)   my_strcat("or\n", dst, dst_size);
}

void compile_varref(int index, char* dst, int dst_size){
    char tmp[12];
    VarLookup i1 = ref_var( nodes[index].varref.id );

    if(i1.table==TABLE_FUNC){    //argument, local
        if(func_var_table[i1.index].segment==SEG_ARG) my_strcat("push argument ", dst, dst_size);
        else    my_strcat("push local ", dst, dst_size);
        my_int2str(func_var_table[i1.index].index, tmp, 12);
    }else{  //field, static
        if(class_var_table[i1.index].segment==SEG_FIELD) my_strcat("push this ", dst, dst_size);
        else    my_strcat("push static ", dst, dst_size);
        my_int2str(class_var_table[i1.index].index, tmp, 12);
    }
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);
    
}

void compile_index(int index, char* dst, int dst_size){

    //ベースアドレス
    char tmp[12];
    VarLookup i1 = ref_var( nodes[ nodes[index].index.id ].varref.id );

    if(i1.table==TABLE_FUNC){    //argument, local
        if(func_var_table[i1.index].segment==SEG_ARG) my_strcat("push argument ", dst, dst_size);
        else    my_strcat("push local ", dst, dst_size);
        my_int2str(func_var_table[i1.index].index, tmp, 12);
    }else{  //field, static
        if(class_var_table[i1.index].segment==SEG_FIELD) my_strcat("push this ", dst, dst_size);
        else    my_strcat("push static ", dst, dst_size);
        my_int2str(class_var_table[i1.index].index, tmp, 12);
    }
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    //インデックス
    compile_expr(nodes[index].index.index, dst, dst_size);

    my_strcat("add\npop pointer 1\n", dst, dst_size);
    my_strcat("push that 0\n", dst, dst_size);

}

void compile_call(int index, char* dst, int dst_size){
    int arg_index = nodes[index].call.arg_first;
    char tmp[12];

    //methodのインスタンス
    if( nodes[index].call.instance != -1 ){
        VarLookup i1 = ref_var( nodes[nodes[index].call.instance].varref.id );
        if(i1.table==TABLE_FUNC){    //argument, local
            if(func_var_table[i1.index].segment==SEG_ARG) my_strcat("push argument ", dst, dst_size);
            else    my_strcat("push local ", dst, dst_size);
            my_int2str(func_var_table[i1.index].index, tmp, 12);
        }else{  //field, static
            if(class_var_table[i1.index].segment==SEG_FIELD) my_strcat("push this ", dst, dst_size);
            else    my_strcat("push static ", dst, dst_size);
            my_int2str(class_var_table[i1.index].index, tmp, 12);
        }
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);

        nodes[index].call.arg_count++;
    }

    //引数のセット
    for(int i=0; i<nodes[index].call.arg_count; i++){
        compile_expr( arg_index, dst, dst_size );
        arg_index = nodes[arg_index].next;
    }

    my_strcat("call ", dst, dst_size);
    if(nodes[index].call.instance != -1){
        nodes[index].call.type = ref_type(nodes[nodes[index].call.instance].varref.id);
    }
    my_strcat( name_table[nodes[index].call.type].name, dst, dst_size);
    my_strcat(".", dst, dst_size);
    my_strcat( name_table[nodes[index].call.func_id].name, dst, dst_size);
    my_strcat(" ", dst, dst_size);
    my_int2str( nodes[index].call.arg_count, tmp, 12);
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
    func_var_table[func_table_last].name_id = nodes[index].vardec.id;
    func_var_table[func_table_last].type = nodes[index].vardec.type;
    func_var_table[func_table_last].segment = 1;
    func_var_table[func_table_last].index = local_index;
    func_table_last++;

    //配列
    if( nodes[index].vardec.array_size != -1 ){
        //配列サイズ
        my_strcat("push const ", dst, dst_size);
        my_int2str( nodes[index].vardec.array_size, tmp, 12 );
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);

        //アドレス取得
        my_strcat("call $alloc 1\n", dst, dst_size);

        //アドレス代入
        my_strcat("pop local ", dst, dst_size);
        
        my_int2str(local_index, tmp1, 12);
        my_strcat(tmp1, dst, dst_size);
        my_strcat("\n", dst, dst_size);

    }

    //初期値
    if( nodes[index].vardec.expr != -1 ){

        if( nodes[index].vardec.array_size == -1 ){    //スカラ

            //初期値セット
            compile_expr( nodes[index].vardec.expr, dst, dst_size );

            //代入
            char tmp[12];
            VarLookup i1 = ref_var( nodes[index].vardec.id );

            if(i1.table==TABLE_FUNC){    //argument, local
                if(func_var_table[i1.index].segment==SEG_ARG) my_strcat("pop argument ", dst, dst_size);
                else    my_strcat("pop local ", dst, dst_size);
                my_int2str(func_var_table[i1.index].index, tmp, 12);
            }else{  //field, static
                if(class_var_table[i1.index].segment==SEG_FIELD) my_strcat("pop this ", dst, dst_size);
                else    my_strcat("pop static ", dst, dst_size);
                my_int2str(class_var_table[i1.index].index, tmp, 12);
            }
            my_strcat(tmp, dst, dst_size);
            my_strcat("\n", dst, dst_size);

        }else{  //配列

            int init = nodes[index].vardec.expr;

            std::cout << "debug1:" << std::endl;
            for(int i=0; i<nodes[index].vardec.array_size; i++){

                //初期値セット
                compile_expr( init, dst, dst_size );

                //ベースアドレス
                char tmp[12];
                my_strcat("push local ", dst, dst_size);
                my_int2str(func_var_table[ ref_func_var( nodes[index].vardec.id ) ].index, tmp, 12);
                my_strcat(tmp, dst, dst_size);
                my_strcat("\n", dst, dst_size);

                //インデックス
                my_strcat("push const ", dst, dst_size);
                my_int2str(i, tmp, 12);
                my_strcat(tmp, dst, dst_size);
                my_strcat("\n", dst, dst_size);

                //代入
                my_strcat("add\npop pointer 1\npop that 0\n", dst, dst_size);

                init = nodes[init].next;
            }

        }

    }

    local_index++;
}

void compile_class_vardec(int index, char* dst, int dst_size){

    char tmp[12];
    char tmp1[12];

    //変数テーブルに登録
    class_var_table[class_table_last].name_id = nodes[index].vardec.id;
    class_var_table[class_table_last].type = nodes[index].vardec.type;
    class_var_table[class_table_last].segment = nodes[index].vardec.is_static;
    if(nodes[index].vardec.is_static)  class_var_table[class_table_last].index = static_index++;
    else class_var_table[class_table_last].index = field_index++;
    class_table_last++;

}

void compile_varset(int index, char* dst, int dst_size){

    //代入する値
    compile_expr( nodes[index].varset.expr, dst, dst_size);

    //代入
    if( nodes[nodes[index].varset.target].type == NODE_VARREF ){
        char tmp[12];
        VarLookup i1 = ref_var( nodes[nodes[index].varset.target].varref.id );

        if(i1.table==TABLE_FUNC){    //argument, local
            if(func_var_table[i1.index].segment==SEG_ARG) my_strcat("pop argument ", dst, dst_size);
            else    my_strcat("pop local ", dst, dst_size);
            my_int2str(func_var_table[i1.index].index, tmp, 12);
        }else{  //field, static
            if(class_var_table[i1.index].segment==SEG_FIELD) my_strcat("pop this ", dst, dst_size);
            else    my_strcat("pop static ", dst, dst_size);
            my_int2str(class_var_table[i1.index].index, tmp, 12);
        }
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);

    }else{  //配列

        //ベースアドレス
        char tmp[12];
        VarLookup i1 = ref_var( nodes[ nodes[ nodes[index].varset.target ].index.id ].varref.id );

        if(i1.table==TABLE_FUNC){    //argument, local
            if(func_var_table[i1.index].segment==SEG_ARG) my_strcat("push argument ", dst, dst_size);
            else    my_strcat("push local ", dst, dst_size);
            my_int2str(func_var_table[i1.index].index, tmp, 12);
        }else{  //field, static
            if(class_var_table[i1.index].segment==SEG_FIELD) my_strcat("push this ", dst, dst_size);
            else    my_strcat("push static ", dst, dst_size);
            my_int2str(class_var_table[i1.index].index, tmp, 12);
        }
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);

        //インデックス
        compile_expr(nodes[ nodes[index].varset.target ].index.index, dst, dst_size);

        my_strcat("add\npop pointer 1\n", dst, dst_size);

        //代入
        my_strcat("pop that 0\n", dst, dst_size);

    }
}

void compile_if(int index, char* dst, int dst_size){

    char tmp[12] = "";
    my_int2str(++label_count, tmp, 12);

    //判定式
    compile_expr( nodes[index].if_.cond, dst, dst_size );

    my_strcat("ifgo ", dst, dst_size);
    my_strcat("if_then_", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    //else
    compile_statement_list( nodes[index].if_.else_, dst, dst_size );

    my_strcat("goto ", dst, dst_size);
    my_strcat("if_end_", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    my_strcat("label ", dst, dst_size);
    my_strcat("if_then_", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    //then
    compile_statement_list( nodes[index].if_.then_, dst, dst_size );

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
    compile_expr( nodes[index].while_.cond, dst, dst_size);

    my_strcat("not\nifgo loopen", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    //処理
    compile_statement_list( nodes[index].while_.then_, dst, dst_size );
    
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
    compile_vardec( nodes[index].for_.init, dst, dst_size);

    my_strcat("label forst", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    //判定式
    compile_expr( nodes[index].for_.cond, dst, dst_size);

    my_strcat("not\nifgo loopen", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    //処理
    compile_statement_list( nodes[index].for_.body, dst, dst_size);

    //末尾の処理
    compile_statement( nodes[index].for_.update, dst, dst_size );

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
    compile_expr( nodes[index].return_.expr, dst, dst_size);
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

    func_table_last = 0;
    argument_index = 0;
    local_index = 0;
    local_index_tmp = 0;

    //関数宣言
    my_strcat("\nfunction ", dst, dst_size);
    my_strcat(name_table[nodes[current_class_id].func.id].name, dst, dst_size);
    my_strcat(".", dst, dst_size);
    my_strcat(name_table[nodes[index].func.id].name, dst, dst_size);
    my_strcat(" ", dst, dst_size);
    my_int2str(nodes[index].func.local_count, tmp, 64);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);

    //constructor (alloc)
    if(nodes[index].func.kind == CONSTRUCTOR){
        //this
        my_strcat("push const ", dst, dst_size);
        my_int2str(nodes[current_class_id].class_.field_count, tmp, 12);
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n", dst, dst_size);
        my_strcat("call $alloc 1\npop pointer 0\n", dst, dst_size);

        //field 配列
        int node = nodes[current_class_id].func.body;
        while(node != -1){
            if(nodes[node].vardec.type == NODE_VARDEC && nodes[node].vardec.is_static==0 && nodes[node].vardec.array_size!=-1){
                my_strcat("push const ", dst, dst_size);    //配列サイズ
                my_int2str(nodes[node].vardec.array_size, tmp, 12);
                my_strcat(tmp, dst, dst_size);
                my_strcat("\ncall $alloc 1\n", dst, dst_size);

                my_strcat("pop this ", dst, dst_size);    //staticインデックス
                int idx = ref_class_var(nodes[node].vardec.id);
                my_int2str(class_var_table[idx].index, tmp, 12);
                my_strcat(tmp, dst, dst_size);
                my_strcat("\n", dst, dst_size);
            }
            node = nodes[node].next;
        }
    }

    //method
    if(nodes[index].func.kind == METHOD){
        my_strcat("push argument 0\npop pointer 0\n", dst, dst_size);
    }

    //引数セット
    int arg_index = nodes[index].func.arg_first;
    for(int i=0; i<nodes[index].func.arg_count; i++){
        func_var_table[func_table_last].name_id = nodes[arg_index].arg.id;
        func_var_table[func_table_last].type = nodes[arg_index].arg.type;
        func_var_table[func_table_last].segment = 0;
        func_var_table[func_table_last].index = argument_index++;
        if(nodes[index].func.kind == 1)  func_var_table[func_table_last].index++;    //methodならば引数を1ずらす
        func_table_last++;
        arg_index = nodes[arg_index].next;
    }

    //文
    int statement_index = nodes[index].func.body;
    compile_statement_list( statement_index, dst, dst_size );

    //constructor (return this)
    if(nodes[index].func.kind == CONSTRUCTOR){
        my_strcat("push pointer 0\nreturn\n", dst, dst_size);
    }else{  //強制return 0
        my_strcat("push const 0\nreturn\n", dst, dst_size);
    }
}

void compile_class(int index, char* dst, int dst_size){

    char tmp[64] = "";

    class_table_last = 0;
    field_index = 0;
    current_class_id = index;

    int index1 = nodes[index].class_.body;

    while(index1 != -1){
        if( nodes[index1].type == NODE_VARDEC ) compile_class_vardec(index1, dst, dst_size);
        else    compile_func(index1, dst, dst_size);

        index1 = nodes[index1].next;
    }

}

void compile_class_static_alloc(int index, char* dst, int dst_size){

    static_index = 0;

    int index1;
    while(index != -1){
        index1 = nodes[index].class_.body;
        while(index1!=-1){
            if( nodes[index1].type == NODE_VARDEC && nodes[index1].vardec.is_static){
                if(nodes[index1].vardec.array_size!=-1){
                     
                    char tmp1[12];

                    //配列サイズ
                    my_strcat("push const ", dst, dst_size);
                    my_int2str( nodes[index1].vardec.array_size, tmp1, 12 );
                    my_strcat(tmp1, dst, dst_size);
                    my_strcat("\n", dst, dst_size);

                    //アドレス取得
                    my_strcat("call $alloc 1\n", dst, dst_size);

                    // アドレス代入
                    my_strcat("pop static ", dst, dst_size);
                    
                    my_int2str(static_index, tmp1, 12);
                    my_strcat(tmp1, dst, dst_size);
                    my_strcat("\n", dst, dst_size);

                }
                static_index++;
            }
            index1 = nodes[index1].next;
        }
        index = nodes[index].next;
    }

    static_index = 0;

}



VM vm;


int main(){

    my_strcpy(jack_script0, jack_script, 100000);

    //コメント除去
    remove_comment();

    //トークナイズ
    char output[100000] = "";
    char tmp[128];

    tokenize_index = 0;

    tokenize_index = 0;
    output[0] = '\0';

    get_type_name();    //型名の取得

    int root_node = parse_main();

    std::cout << "\nnodes :" << std::endl;
    print_main(root_node, 0);

    std::cout << "\nname table :" << std::endl;
    for(int i=0; i<name_table_index; i++){
        if(name_table[i].is_type)   std::cout << i << " :*" << name_table[i].name << std::endl;
        else std::cout << i << " : " << name_table[i].name << std::endl;
    }


    
    
    // if(error_line != -1){
    //     std::cout << "\nsyntax error : " << std::endl;
    //     std::cout << "line : " << error_line << std::endl;
    //     source_of_line(error_line, output, 100000);
    //     std::cout << output << std::endl;
    // }else if(true){
    //     //コンパイル
    //     output[0] = '\0';

    //     compile_class_static_alloc(root_node, output, 100000);
    //     my_strcat(vm_header, output, 100000);
    //     compile_main(root_node, output, 100000);
    //     my_strcat(vm_footer, output, 100000);

    //     std::cout << std::endl;
    //     std::cout << "class var table : " << std::endl;
    //     std::cout << "name,type,sement,index" << std::endl;
    //     for(int i=0; i<class_table_last; i++){
    //         std::cout << class_var_table[i].name_id << " " << class_var_table[i].type << " " << class_var_table[i].segment << " " << class_var_table[i].index << std::endl;
    //     }

    //     std::cout << std::endl;
    //     std::cout << "func var table : " << std::endl;
    //     std::cout << "name,type,sement,index" << std::endl;
    //     for(int i=0; i<func_table_last; i++){
    //         std::cout << func_var_table[i].name_id << " " << func_var_table[i].type << " " << func_var_table[i].segment << " " << func_var_table[i].index << std::endl;
    //     }

    //     std::cout << std::endl;
    //     std::cout << "vm script : " << std::endl;
    //     std::cout << output << std::endl;

    //     //実行
    //     vm.run(output, output, 100000);

    //     std::cout << "### stack ###" << std::endl;
    //     char s1[] = " : ";
    //     for(int i=0; i<40; i++){
    //         if(i==vm.sp) s1[1] = '#';
    //         else    s1[1] = ':';
    //         std::cout << i << s1 << vm.stack[i] << std::endl;
    //     }

    //     std::cout << std::endl;
    //     std::cout << "### heap ###" << std::endl;
    //     for(int i=0; i<20; i++){
    //         std::cout << i << " : " << vm.heap[i] << std::endl;
    //     }

    //     std::cout << std::endl;
    //     std::cout << "### static ###" << std::endl;
    //     for(int i=0; i<20; i++){
    //         std::cout << i << " : " << vm.statics[i] << std::endl;
    //     }

    //     std::cout << "\noutput : \n" << output << std::endl;

    // }





    return 0;
}