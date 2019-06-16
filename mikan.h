char *user_input;

typedef struct {
    void **data;
    int capacity;
    int len;
} Vector;


Vector *tokens;
Vector *new_vector();
void vec_push(Vector *vec, void *elem);

void error_at(char *loc, char *msg);

void runtest();


typedef struct Node {
    int ty;
    struct Node *lhs;
    struct Node *rhs;
    int val;
} Node;

enum {
    ND_NUM = 256,
    ND_EQ,
    ND_NE,
    ND_LE,
    ND_GE,
};

Node *expr();
void gen(Node *node);


typedef struct {
    int ty;
    int val;
    char *input;
} Token;

enum {
    TK_NUM = 256,
    TK_EQ,
    TK_NE,
    TK_LE,
    TK_GE,
    TK_EOF,
};

Vector *tokenize();