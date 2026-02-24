#include "compiler.hpp"
#include "string_utils.hpp"
#include <iostream>


void Compiler::run(const char* src, char* dst, int dst_size){

    int root_node;

    dst[0] = '\0';

    
    my_strcpy(src, jack_script, Config::SOURCE_SIZE);    //スクリプト代入
    remove_comment();   //コメント除去
    initialize();    //初期化
    root_node = parse_main();   //AST生成

    if(error_line!=-1){ //syntax error
        my_strcat("syntax error", error_message, Config::ERROR_SIZE);
    }else{  //コンパイル
        compile_class_static_alloc(root_node, dst, dst_size);
        my_strcat(vm_header, dst, dst_size);
        compile_main(root_node, dst, dst_size);
        my_strcat(vm_footer, dst, dst_size);
    }

    if(error_line != -1){
        dst[0] = '\0';
        my_strcat("Error at line ", dst, dst_size);
        char tmp[128];
        my_int2str(error_line, tmp, 12);
        my_strcat(tmp, dst, dst_size);
        my_strcat(":\n", dst, dst_size);

        my_strcat(error_message, dst, dst_size);
        my_strcat("\n|\n|", dst, dst_size);
       
        source_of_line(error_line, tmp, 128);
        my_strcat(tmp, dst, dst_size);
        my_strcat("\n|", dst, dst_size);
    }

    //デバッグ用
    // std::cout << "\nnodes :" << std::endl;
    // print_main(root_node, 0);

    // std::cout << std::endl;
    // std::cout << "class var table : " << std::endl;
    // std::cout << "name,type,sement,index" << std::endl;
    // for(int i=0; i<class_table_last; i++){
    //     std::cout << class_var_table[i].name_id << " " << class_var_table[i].type << " " << class_var_table[i].segment << " " << class_var_table[i].index << std::endl;
    // }

    // std::cout << std::endl;
    // std::cout << "func var table : " << std::endl;
    // std::cout << "name,type,sement,index" << std::endl;
    // for(int i=0; i<func_table_last; i++){
    //     std::cout << func_var_table[i].name_id << " " << func_var_table[i].type << " " << func_var_table[i].segment << " " << func_var_table[i].index << std::endl;
    // }

}



//##################################################################
//　　前処理
//##################################################################

void Compiler::remove_comment(){

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

//intとクラスの名前をname_tableにセット
void Compiler::initialize(){

    tokenize_index = 0;
    error_line = -1;
    node_index = 0;
    name_table_index = 0;
    static_index = 0;
    error_message[0] = '\0';

    
    for(int i=0; i<Config::NAME_TABLE_MAX; i++){
        name_table[i].name[0] = '\0';
        name_table[i].is_type = false;
    }

    my_strcpy( "int", name_table[name_table_index].name, 64);   //0
    name_table[name_table_index++].is_type = true;

    my_strcpy("Output", name_table[name_table_index].name, 64); //1
    name_table[name_table_index++].is_type = true;

    my_strcpy("Memory", name_table[name_table_index].name, 64); //2
    name_table[name_table_index++].is_type = true;

    my_strcpy("printi", name_table[name_table_index++].name, 64); //3
    my_strcpy("printc", name_table[name_table_index++].name, 64); //4
    my_strcpy("alloc", name_table[name_table_index++].name, 64);  //5

    
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


    //内部ノードの設定
    node_index = 0;
    new_func(3, NODE_FUNC, -1, 1, 0, -1, 1);  //Output.printi
    new_func(4, NODE_FUNC, -1, 1, 0, -1, 1);  //Output.printc
    new_func(5, NODE_FUNC, -1, 1, 0, -1, 2);  //Memory.alloc

}


//##################################################################
//　　ノード生成
//##################################################################

int Compiler::new_int(int value){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_INT;
    nodes[idx].int_.value = value;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_unary(int op, int left){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_UNARY;
    nodes[idx].unary.op = op;
    nodes[idx].unary.left = left;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_binary(int op, int left, int right){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_BINARY;
    nodes[idx].binary.op = op;
    nodes[idx].binary.left = left;
    nodes[idx].binary.right = right;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_varref(int id){
    int idx = node_index++;
    nodes[idx].type = NODE_VARREF;
    nodes[idx].varref.id = id;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_index(int varid, int index){
    int idx = node_index++;
    nodes[idx].type = NODE_INDEX;
    nodes[idx].index.id = varid;
    nodes[idx].index.index = index;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_argdec(int id, int type){
    int idx = node_index++;
    nodes[idx].type = NODE_ARG;
    nodes[idx].arg.id = id;
    nodes[idx].arg.type = type;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_vardec(int id, int type, int is_static, int array_size, int init){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_VARDEC;
    nodes[idx].vardec.id = id;
    nodes[idx].vardec.type = type;
    nodes[idx].vardec.is_static = is_static;
    nodes[idx].vardec.array_size = array_size;
    nodes[idx].vardec.expr = init;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_varset(int id, int expr){
    int idx = node_index;
    node_index++;
    nodes[idx].type = NODE_VARSET;
    nodes[idx].varset.target = id;
    nodes[idx].varset.expr = expr;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_if(int cond, int then_index, int else_index){
    int idx = node_index++;
    nodes[idx].type = NODE_IF;
    nodes[idx].if_.cond = cond;
    nodes[idx].if_.then_ = then_index;
    nodes[idx].if_.else_ = else_index;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_while(int cond, int then_index){
    int idx = node_index++;
    nodes[idx].type = NODE_WHILE;
    nodes[idx].while_.cond = cond;
    nodes[idx].while_.then_ = then_index;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_for(int init, int cond, int update, int stmt){
    int idx = node_index++;
    nodes[idx].type = NODE_FOR;
    nodes[idx].for_.init = init;
    nodes[idx].for_.cond = cond;
    nodes[idx].for_.update = update;
    nodes[idx].for_.body = stmt;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_call(int id, int type, int arg_index, int arg_count, int instance){
    int idx = node_index++;
    nodes[idx].type = NODE_CALL;
    nodes[idx].call.func_id = id;
    nodes[idx].call.type = type;
    nodes[idx].call.arg_first = arg_index;
    nodes[idx].call.arg_count = arg_count;
    nodes[idx].call.instance = instance;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_break(){
    int idx = node_index++;
    nodes[idx].type = NODE_BREAK;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_return(int expr){
    int idx = node_index++;
    nodes[idx].type = NODE_RETURN;
    nodes[idx].return_.expr = expr;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_func(int id, int kind, int arg_first, int arg_count, int local_count, int body, int parent){
    int idx = node_index++;
    nodes[idx].type = NODE_FUNC;
    nodes[idx].func.kind = kind;
    nodes[idx].func.id = id;
    nodes[idx].func.arg_first = arg_first;
    nodes[idx].func.arg_count = arg_count;
    nodes[idx].func.local_count = local_count;
    nodes[idx].func.body = body;
    nodes[idx].func.parent = parent;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}

int Compiler::new_class(int id, int field_count, int body){
    int idx = node_index++;
    nodes[idx].type = NODE_CLASS;
    nodes[idx].class_.id = id;
    nodes[idx].class_.field_count = field_count;
    nodes[idx].class_.body = body;
    nodes[idx].next = -1;
    nodes[idx].token = tokenize_index;
    return idx;
}


//##################################################################
//　　AST生成
//##################################################################

//トークンインデックスからjack_scriptの行数を返す
int Compiler::line_of_token(int index){
    int result = 0;
    for(int i=0; i<index; i++){
        if(jack_script[i] == '\n')  result++;
    }
    return result + 1;
}

//行数からjack_scriptのその行の文字列を返す
void Compiler::source_of_line(int line, char* dst, int dst_size){
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

void Compiler::nextToken(char* dst, int dst_size, bool commit){

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

void Compiler::nnextToken(char* dst, int dst_size){
    int tmp = tokenize_index;
    nextToken(dst, dst_size, true);
    nextToken(dst, dst_size, false);
    tokenize_index = tmp;
}

void Compiler::expect(const char* src){
    if(error_line == -1){
        int tokenize_index_copy = tokenize_index;
        char tmp[64];
        nextToken(tmp, 64, true);
        if( !my_streq(src,tmp) ){
            error_line = line_of_token(tokenize_index);
        }
    }
}


int Compiler::set_name_table(const char* src, bool is_type){

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

int Compiler::ref_name_table(const char* src){
    for(int i=0; i<name_table_index; i++){
        if(my_streq(src, name_table[i].name))    return i;
    }
    return -1;
}

bool Compiler::istype_name_table(const char* src){
    for(int i=0; i<name_table_index; i++){
        if(my_streq(src, name_table[i].name))   return name_table[i].is_type;
    }
    return false;
}


int Compiler::parse_main(){
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

int Compiler::parse_expr(){
    if(error_line != -1)    return -1;
    return parse_logical_or();
}

int Compiler::parse_logical_or(){
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

int Compiler::parse_logical_and(){
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

int Compiler::parse_equality(){
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

int Compiler::parse_relational(){
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

int Compiler::parse_add_sub(){
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

int Compiler::parse_mul_div(){
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

int Compiler::parse_unary(){
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

int Compiler::parse_primary(){
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

int Compiler::parse_lvalue(){

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


int Compiler::parse_statement_list(){
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

int Compiler::parse_statement(){
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

int Compiler::parse_class_vardec(){
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

int Compiler::parse_vardec(){
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

int Compiler::parse_varset(int target){
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

int Compiler::parse_if(){
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

int Compiler::parse_while(){
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

int Compiler::parse_for(){
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

int Compiler::parse_call(){
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

int Compiler::parse_method(int instance){
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

int Compiler::parse_break(){
    if(error_line != -1)    return -1;
    expect("break");
    expect(";");
    return new_break();
}

int Compiler::parse_return(){
    if(error_line != -1)    return -1;
    expect("return");
    int expr = parse_expr();
    expect(";");
    return new_return(expr);
}


int Compiler::parse_func(int class_id){
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
    
    return new_func(func_id, kind, arg_origin, arg_count, local_count, body_id, class_id);
}

int Compiler::parse_class(){
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
            body_id = parse_func(name_id);
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

void Compiler::print_main(int index, int nest){
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

void Compiler::print_int(int index, int nest){
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

void Compiler::print_unary(int index, int nest){
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

void Compiler::print_binary(int index, int nest){
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

void Compiler::print_varref(int index, int nest){
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

void Compiler::print_index(int index, int nest){
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

void Compiler::print_argdec(int index, int nest){
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

void Compiler::print_vardec(int index, int nest){
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

void Compiler::print_varset(int index, int nest){
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

void Compiler::print_if(int index, int nest){
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

void Compiler::print_while(int index, int nest){
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

void Compiler::print_for(int index, int nest){
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

void Compiler::print_call(int index, int nest){
    
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

void Compiler::print_break(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output, 128);
    }
    my_strcat("BREAK", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].next, nest);
}

void Compiler::print_return(int index, int nest){
    char output[128] = "";
    for(int i=0; i<nest; i++){
        my_strcat("  ", output ,128);
    }
    my_strcat("RETURN", output, 128);
    std::cout << output << std::endl;

    print_main(nodes[index].return_.expr, nest+1);
    print_main(nodes[index].next, nest);
}

void Compiler::print_func(int index, int nest){
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

void Compiler::print_class(int index, int nest){
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

int Compiler::ref_func_var(int name_id){
    for(int i=0; i<func_table_last; i++){
        if(func_var_table[i].name_id == name_id){
            return i;
        }
    }
    return -1;
}

int Compiler::ref_class_var(int name_id){
    for(int i=0; i<class_table_last; i++){
        if(class_var_table[i].name_id == name_id){
            return i;
        }
    }
    return -1;
}

//x: (0:func, 1:class), y:index
Compiler::VarLookup Compiler::ref_var(int name_id){
    VarLookup result;
    result.table = 0;
    result.index = ref_func_var(name_id);
    if(result.index != -1)  return result;

    result.table = 1;
    result.index = ref_class_var(name_id);

    return result;
}

int Compiler::ref_type(int name_id){
    for(int i=0; i<func_table_last; i++){
        if(func_var_table[i].name_id == name_id)    return func_var_table[i].type;
    }
    for(int i=0; i<class_table_last; i++){
        if(class_var_table[i].name_id == name_id)   return class_var_table[i].type;
    }
    return -1;
}

int Compiler::get_argument_count(int func, int type){
    for(int i=0; i<node_index; i++){
        if(nodes[i].type==NODE_FUNC && nodes[i].func.id==func && nodes[i].func.parent==type){
            return nodes[i].func.arg_count;
        }
    }
    return -1;
}


void Compiler::compile_main(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    while(index != -1){
        compile_class(index, dst, dst_size);
        index = nodes[index].next;
    }

}

void Compiler::compile_int(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    char tmp[12] = "";
    my_strcat("push const ", dst, dst_size);
    my_int2str(nodes[index].int_.value, tmp, 12);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);
}

void Compiler::compile_unary(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    compile_expr( nodes[index].unary.left, dst, dst_size );
    if(nodes[index].unary.op == OP_NEG)   my_strcat("neg\n", dst, dst_size);
    if(nodes[index].unary.op == OP_NOT)   my_strcat("not\n", dst, dst_size);
}

void Compiler::compile_binary(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
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

void Compiler::compile_varref(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    char tmp[12];
    VarLookup i1 = ref_var( nodes[index].varref.id );

    if(i1.index == -1){
        error_line = line_of_token(nodes[index].token);
        my_strcat("\'", error_message, Config::ERROR_SIZE);
        my_strcat(name_table[nodes[index].varref.id].name, error_message, Config::ERROR_SIZE);
        my_strcat("\'", error_message, Config::ERROR_SIZE);
        my_strcat(" is not defined.", error_message, Config::ERROR_SIZE);
        return;
    }

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

void Compiler::compile_index(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    //ベースアドレス
    char tmp[12];
    VarLookup i1 = ref_var( nodes[ nodes[index].index.id ].varref.id );

    if(i1.index == -1){
        error_line = line_of_token(nodes[index].token);
        my_strcat("\'", error_message, Config::ERROR_SIZE);
        my_strcat(name_table[nodes[ nodes[index].index.id ].varref.id].name, error_message, Config::ERROR_SIZE);
        my_strcat("\'", error_message, Config::ERROR_SIZE);
        my_strcat(" is not defined.", error_message, Config::ERROR_SIZE);
        return;
    }

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

void Compiler::compile_call(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    int arg_index = nodes[index].call.arg_first;
    char tmp[12];

    //methodのインスタンス
    if( nodes[index].call.instance != -1 ){
        VarLookup i1 = ref_var( nodes[nodes[index].call.instance].varref.id );

        //未定エラー
        if(i1.index == -1){
            error_line = line_of_token(nodes[index].token);
            my_strcat("\'", error_message, Config::ERROR_SIZE);
            my_strcat(name_table[nodes[nodes[index].call.instance].varref.id].name, error_message, Config::ERROR_SIZE);
            my_strcat("\'", error_message, Config::ERROR_SIZE);
            my_strcat(" is not defined.", error_message, Config::ERROR_SIZE);
            return;
        }

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

        nodes[index].call.type = ref_type(nodes[nodes[index].call.instance].varref.id);
    }

    //関数が存在するかチェック
    int func_arg_count = get_argument_count(nodes[index].call.func_id, nodes[index].call.type);
    if(func_arg_count == -1){
        error_line = line_of_token(nodes[index].token);
        my_strcat("\'", error_message, Config::ERROR_SIZE);
        my_strcat(name_table[nodes[index].call.func_id].name, error_message, Config::ERROR_SIZE);
        my_strcat("\'", error_message, Config::ERROR_SIZE);
        my_strcat(" is not defined.", error_message, Config::ERROR_SIZE);
        return;
    }
    if(func_arg_count != nodes[index].call.arg_count){
        error_line = line_of_token(nodes[index].token);
        my_strcat("\'", error_message, Config::ERROR_SIZE);
        my_strcat(name_table[nodes[index].call.func_id].name, error_message, Config::ERROR_SIZE);
        my_strcat("\'", error_message, Config::ERROR_SIZE);
        my_strcat(" expects ", error_message, Config::ERROR_SIZE);
        my_int2str(func_arg_count, tmp, 12);
        if(func_arg_count>0)    my_strcat(tmp, error_message, Config::ERROR_SIZE);
        if(func_arg_count>1)    my_strcat(" argument.", error_message, Config::ERROR_SIZE);
        if(func_arg_count==1)   my_strcat(" argument.", error_message, Config::ERROR_SIZE);
        if(func_arg_count==0)   my_strcat("no argument.", error_message, Config::ERROR_SIZE);
        return;
    }

    //method
    if(nodes[index].call.instance != -1){
        nodes[index].call.arg_count++;
    }

    //引数のセット
    for(int i=0; i<nodes[index].call.arg_count; i++){
        compile_expr( arg_index, dst, dst_size );
        arg_index = nodes[arg_index].next;
    }

    my_strcat("call ", dst, dst_size);
    my_strcat( name_table[nodes[index].call.type].name, dst, dst_size);
    my_strcat(".", dst, dst_size);
    my_strcat( name_table[nodes[index].call.func_id].name, dst, dst_size);
    my_strcat(" ", dst, dst_size);
    my_int2str( nodes[index].call.arg_count, tmp, 12);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);
}

void Compiler::compile_expr(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    if(index == -1) return;
    if(nodes[index].type == NODE_BINARY)    compile_binary(index, dst, dst_size);
    if(nodes[index].type == NODE_UNARY)    compile_unary(index, dst, dst_size);
    if(nodes[index].type == NODE_INT)   compile_int(index, dst, dst_size);
    if(nodes[index].type == NODE_VARREF)    compile_varref(index, dst, dst_size);
    if(nodes[index].type == NODE_INDEX) compile_index(index, dst, dst_size);
    if(nodes[index].type == NODE_CALL)  compile_call(index, dst, dst_size);
    return;
}

void Compiler::compile_vardec(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    char tmp[12];
    char tmp1[12];

    //変数重複チェック
    VarLookup i1 = ref_var(nodes[index].vardec.id);
    if(i1.index != -1){
        error_line = line_of_token(nodes[index].token);
        my_strcat("\'", error_message, Config::ERROR_SIZE);
        my_strcat(name_table[nodes[index].vardec.id].name, error_message, Config::ERROR_SIZE);
        my_strcat("\'", error_message, Config::ERROR_SIZE);
        my_strcat(" is already defined.", error_message, Config::ERROR_SIZE);
    }

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

void Compiler::compile_class_vardec(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    char tmp[12];
    char tmp1[12];

    //変数重複チェック
    VarLookup i1 = ref_var(nodes[index].vardec.id);
    if(i1.index != -1){
        error_line = line_of_token(nodes[index].token);
        my_strcat("\'", error_message, Config::ERROR_SIZE);
        my_strcat(name_table[nodes[index].vardec.id].name, error_message, Config::ERROR_SIZE);
        my_strcat("\'", error_message, Config::ERROR_SIZE);
        my_strcat(" is already defined.", error_message, Config::ERROR_SIZE);
    }

    //変数テーブルに登録
    class_var_table[class_table_last].name_id = nodes[index].vardec.id;
    class_var_table[class_table_last].type = nodes[index].vardec.type;
    class_var_table[class_table_last].segment = nodes[index].vardec.is_static;
    if(nodes[index].vardec.is_static)  class_var_table[class_table_last].index = static_index++;
    else class_var_table[class_table_last].index = field_index++;
    class_table_last++;

}

void Compiler::compile_varset(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    //代入する値
    compile_expr( nodes[index].varset.expr, dst, dst_size);

    //代入
    if( nodes[nodes[index].varset.target].type == NODE_VARREF ){
        char tmp[12];
        VarLookup i1 = ref_var( nodes[nodes[index].varset.target].varref.id );

        if(i1.index == -1){
            error_line = line_of_token(nodes[index].token);
            my_strcat("\'", error_message, Config::ERROR_SIZE);
            my_strcat(name_table[nodes[nodes[index].varset.target].varref.id].name, error_message, Config::ERROR_SIZE);
            my_strcat("\'", error_message, Config::ERROR_SIZE);
            my_strcat(" is not defined.", error_message, Config::ERROR_SIZE);
            return;
        }

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

void Compiler::compile_if(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
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

void Compiler::compile_while(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
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

void Compiler::compile_for(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    int local_index_tmp = local_index;
    int saved_last = func_table_last;
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
    func_table_last = saved_last;
}

void Compiler::compile_return(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    //返り値
    compile_expr( nodes[index].return_.expr, dst, dst_size);
    //リターン
    my_strcat("return\n", dst, dst_size);
}

void Compiler::compile_break(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    char tmp[12] = "";
    my_int2str(label_count, tmp, 12);
    my_strcat("goto loopen", dst, dst_size);
    my_strcat(tmp, dst, dst_size);
    my_strcat("\n", dst, dst_size);
}

void Compiler::compile_procedure(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    compile_call(index, dst, dst_size);
    my_strcat("pop temp 0\n", dst, dst_size);
}

void Compiler::compile_statement(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    if(nodes[index].type == NODE_VARDEC)    compile_vardec(index, dst, dst_size);
    if(nodes[index].type == NODE_VARSET)   compile_varset(index, dst, dst_size);
    if(nodes[index].type == NODE_IF)    compile_if(index, dst, dst_size);
    if(nodes[index].type == NODE_WHILE)    compile_while(index, dst, dst_size);
    if(nodes[index].type == NODE_FOR)    compile_for(index, dst, dst_size);
    if(nodes[index].type == NODE_RETURN)    compile_return(index, dst, dst_size);
    if(nodes[index].type == NODE_BREAK)    compile_break(index, dst, dst_size);
    if(nodes[index].type == NODE_CALL)    compile_procedure(index, dst, dst_size);
}

void Compiler::compile_statement_list(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    int local_index_tmp = local_index;
    int saved_last = func_table_last;
    while(index != -1){
        compile_statement(index, dst, dst_size);
        index = nodes[index].next;
    }
    local_index = local_index_tmp;
    func_table_last = saved_last;
}

void Compiler::compile_func(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
    char tmp[64] = "";

    func_table_last = 0;
    argument_index = 0;
    local_index = 0;

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

        //変数重複チェック
        VarLookup i1 = ref_var(nodes[arg_index].arg.id);
        if(i1.index != -1){
            error_line = line_of_token(nodes[arg_index].token);
            my_strcat("\'", error_message, Config::ERROR_SIZE);
            my_strcat(name_table[nodes[arg_index].arg.id].name, error_message, Config::ERROR_SIZE);
            my_strcat("\'", error_message, Config::ERROR_SIZE);
            my_strcat(" is already defined.", error_message, Config::ERROR_SIZE);
        }

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

void Compiler::compile_class(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
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

void Compiler::compile_class_static_alloc(int index, char* dst, int dst_size){
    if(error_line != -1)    return;
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

