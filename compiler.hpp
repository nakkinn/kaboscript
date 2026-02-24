# pragma once


namespace Config{
    const int SOURCE_SIZE = 100000;
    const int ERROR_SIZE = 200;
    const int NODE_MAX = 2000;
    const int NAME_TABLE_MAX = 2000;
    const int VAR_TABLE_MAX = 1000;
}


class Compiler{

public:
    void run(const char* src, char* dst, int dst_size);

private:


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
    struct FuncNode{int id; int kind; int arg_first; int arg_count; int local_count; int body; int parent;};
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

    //##################################################################
    //　　変数
    //##################################################################

    char jack_script[Config::SOURCE_SIZE];  //高級言語コピー（引数を直接参照に移行する予定）
    char error_message[Config::ERROR_SIZE];

    int tokenize_index = 0; //トークナイズ
    int error_line = -1;    //エラー行

    Node nodes[Config::NODE_MAX];   //ASTノード
    int node_index = 0;

    NameEntry name_table[Config::NAME_TABLE_MAX];
    int name_table_index = 0;
    int local_count;
    int current_local;
    int field_count;

    VarEntry func_var_table[Config::VAR_TABLE_MAX];
    int func_table_last = 0;
    int local_index = 0;
    int argument_index = 0;

    VarEntry class_var_table[Config::VAR_TABLE_MAX];
    int class_table_last = 0;
    int field_index = 0;
    int static_index = 0;
    int label_count = 0;
    int current_class_id = -1;


    //##################################################################
    //　　前処理
    //##################################################################

    void remove_comment();
    void initialize();


    //##################################################################
    //　　ノード生成
    //##################################################################

    int new_int(int value);
    int new_unary(int op, int left);
    int new_binary(int op, int left, int right);
    int new_varref(int id);
    int new_index(int varid, int index);
    int new_argdec(int id, int type);
    int new_vardec(int id, int type, int is_static, int array_size, int init);
    int new_varset(int id, int expr);
    int new_if(int cond, int then_index, int else_index);
    int new_while(int cond, int then_index);
    int new_for(int init, int cond, int update, int stmt);
    int new_call(int id, int type, int arg_index, int arg_count, int instance);
    int new_break();
    int new_return(int expr);
    int new_func(int id, int kind, int arg_first, int arg_count, int local_count, int body, int parent);
    int new_class(int id, int field_count, int body);


    //##################################################################
    //　　AST生成
    //##################################################################

    int line_of_token(int index);
    void source_of_line(int line, char* dst, int dst_size);
    void nextToken(char* dst, int dst_size, bool commit);
    void nnextToken(char* dst, int dst_size);
    void expect(const char* src);

    int set_name_table(const char* src, bool is_type);
    int ref_name_table(const char* src);
    bool istype_name_table(const char* src);
    
    int parse_main();
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
    int parse_func(int class_id);
    int parse_class();


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


    //##################################################################
    //　　コンパイル
    //##################################################################

    int ref_func_var(int name_id);
    int ref_class_var(int name_id);
    VarLookup ref_var(int name_id);
    int ref_type(int name_id);
    int get_argument_count(int func, int type); //条件に合う関数の引数を返す、存在しなければ-1を返す

    void compile_main(int index, char* dst, int dst_size);
    void compile_int(int index, char* dst, int dst_size);
    void compile_unary(int index, char* dst, int dst_size);
    void compile_binary(int index, char* dst, int dst_size);
    void compile_varref(int index, char* dst, int dst_size);
    void compile_index(int index, char* dst, int dst_size);
    void compile_call(int index, char* dst, int dst_size);
    void compile_expr(int index, char* dst, int dst_size);
    void compile_vardec(int index, char* dst, int dst_size);
    void compile_class_vardec(int index, char* dst, int dst_size);
    void compile_varset(int index, char* dst, int dst_size);
    void compile_if(int index, char* dst, int dst_size);
    void compile_while(int index, char* dst, int dst_size);
    void compile_for(int index, char* dst, int dst_size);
    void compile_return(int index, char* dst, int dst_size);
    void compile_break(int index, char* dst, int dst_size);
    void compile_procedure(int index, char* dst, int dst_size);
    void compile_statement(int index, char* dst, int dst_size);
    void compile_statement_list(int index, char* dst, int dst_size);
    void compile_func(int index, char* dst, int dst_size);
    void compile_class(int index, char* dst, int dst_size);
    void compile_class_static_alloc(int index, char* dst, int dst_size);

};