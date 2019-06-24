char *user_input;

typedef struct {
    void **data;
    int capacity;
    int len;
} Vector;

Vector *tokens;
Vector *new_vector();
void vec_push(Vector *vec, void *elem);

void error(char *fmt, ...);
void error_at(char *loc, char *msg);

void runtest();

typedef struct {
    Vector *keys;
    Vector *vals;
} Map;

Map *identities;
Map *functions;
Map *new_map();
void map_put(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);
int map_exists(Map *map, char *key);


typedef struct Node {
    int ty;
    struct Node *lhs;
    struct Node *rhs;
    int val;
    int offset;

    //if (cond) then else els
    //while (cond) loop
    //for (init; cond; inc) loop
    struct Node *cond;
    struct Node *then;
    struct Node *els;

    struct Node *loop;

    struct Node *init;
    struct Node *inc;

    struct Vector *block;

    char *name;
    struct Vector *args;
    struct Map *idents;
    int is_return;

    struct Node *parent;
    struct Node *func;

    int var_ty;
    //struct Node *func;
    struct Token *token;

    struct Type *type;
} Node;

typedef struct Type {
    enum {
        INT,
        PTR
    } ty;
    struct Type *ptr_to;
} Type;


enum {
    TY_INT = 256,
};

enum {
    ND_NUM = 256,
    ND_RETURN,
    ND_LVAR,
    ND_IF,
    ND_ELSE,
    ND_WHILE,
    ND_FOR,
    ND_BLOCK,
    ND_FUNC,
    ND_DFUNC,
    ND_EQ,
    ND_NE,
    ND_LE,
    ND_GE,
    ND_INT,
};

Node *code[100];

void program();
void gen(Node *node);

int count;
Map *func_var_count;


typedef struct {
    int ty;
    int val;
    char *name;
    char *input;
} Token;

enum {
    TK_NUM = 256,
    TK_RETURN,
    TK_IDENT,
    TK_IF,
    TK_ELSE,
    TK_WHILE,
    TK_FOR,
    TK_EQ,
    TK_NE,
    TK_LE,
    TK_GE,
    TK_TYPE,
    TK_INT,
    TK_EOF,
};

Vector *tokenize();