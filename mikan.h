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
} Node;

enum {
    ND_NUM = 256,
    ND_RETURN,
    ND_LVAR,
    ND_EQ,
    ND_NE,
    ND_LE,
    ND_GE,
};

Node *code[100];

void program();
void gen(Node *node);


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
    TK_EQ,
    TK_NE,
    TK_LE,
    TK_GE,
    TK_EOF,
};

Vector *tokenize();