#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mikan.h"

int pos = 0;

Node *function();
Node *stmt(Node *parent);
Node *expr(Node *parent);
Node *assign(Node *parent);
Node *equality(Node *parent);
Node *relational(Node *parent);
Node *add(Node *parent);
Node *mul(Node *parent);
Node *term(Node *parent);
Node *unary(Node *parent);

Node *init_func_arg(Node *parent);

int new_func(Node *parent, Node *node);

int consume(int ty) {
    Token *t = tokens->data[pos];

    if (t->ty != ty)
        return 0;
    pos++;
    return 1;
}

int read_ahead(int ty2) {
    Token *t = tokens->data[pos];

    if (t->ty != ty2) {
        return 0;
    }

    return 1;
}

Node *new_node(int ty, Node *lhs, Node *rhs) {
    Node *node = malloc(sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

int new_decl_func(Node *parent, Node *node) {
    if (!consume('('))
        return 0;

    node->args = new_vector();
    node->ty = ND_DFUNC;
    node->block = new_vector();
    node->idents = new_map();
    node->funcs = new_map();
    node->offset = 0;

    while (!consume(')')) {
        vec_push(node->args, init_func_arg(node));
        consume(',');
    }

    Vector *args = node->args;
    for (int j = 0; j < args->len; j++) {
        Node *arg = args->data[j];
        map_put(node->idents, arg->name, arg);
        node->offset = arg->offset;
    }

    if (!consume('{')) {
        printf("error\n");
        return 1;
    }

    while (!consume('}')) {
        vec_push(node->block, stmt(node));
    }

    map_put(parent->funcs, node->name, node);

    return 1;
}

int new_decl_var(Node *parent, Node *node) {
    node->ty = ND_LVAR;
    int size = 0;
    switch (node->type->ty) {
        case INT:
            size = 4;
            break;
        case PTR:
            size = 8;
            break;
        case ARRAY:
            size = node->type->array_size;
            break;
        default:
            return 0;
    }
    parent->offset = parent->offset + size;
    node->offset = parent->offset;
    map_put(parent->idents, node->name, node);

    if (!consume(';'))
        return 0;
    
    return 1;
}

Node *init_type() {
    int ty;

    if (consume(TK_INT)) {
        ty = INT;
    } else {
        //未定義の型
        return NULL;
    }

    Node *node = malloc(sizeof(Node));

    if (consume('*')) {
        Type *t = malloc(sizeof(Type));
        t->ty = PTR;
        node->type = t;
    }

    Type *t = malloc(sizeof(Type));
    t->ty = ty;

    if (node->type != NULL) {
        node->type->ptr_to = t;
    } else {
        node->type = t;
    }

    return node;
}

Node *init_ident(Node *parent, Node *node) {

    if (!consume(TK_IDENT))
        return NULL;

    Token *t = tokens->data[pos-1];
    node->name = t->name;
    node->token = t;
    node->parent = parent;


    if (!consume('[')) {
        return node;
    }

    //配列である場合
    node->type->ty = ARRAY;
    if (!consume(TK_NUM)) {

    }
    
    Token *t_1 = tokens->data[pos-1];
    node->type->array_size = t_1->val + 1;

    if (!consume(']'))
        //error

    return node;
}

Node *init_func_arg(Node *parent) {

    if (!consume(TK_TYPE))
        return NULL;
    Node *node = init_type();
    node = init_ident(parent, node);

    node->ty = ND_DVAR;
    int size = 0;
    switch (node->type->ty) {
        case INT:
            size = 4;
            break;
        case PTR:
            size = 8;
            break;
        case ARRAY:
            size = node->type->array_size;
            break;
        default:
            return 0;
    }
    parent->offset = parent->offset + size;
    node->offset = parent->offset;
    map_put(parent->idents, node->name, node);

    return node;
}

Node *parse_decl(Node *parent) {
    if (!consume(TK_TYPE))
        return NULL;
    Node *node = init_type();
    node = init_ident(parent, node);

    if (new_decl_func(parent, node) == 1) {
        return node;
    }
    
    if (new_decl_var(parent, node) == 1) {
        return node;
    }

    printf("ERROR\n");
}

Node *parse_return(Node *parent) {
    if (!consume(TK_RETURN))
        return NULL;

    Node *node = malloc(sizeof(Node));
    node->ty = ND_RETURN;
    node->lhs = expr(parent);

    if (!consume(';')) {
        Token *t = tokens->data[pos];
        error_at(t->input, "';'ではないトークンです");
    }

    return node;
}

Node *parse_lvar(Node *parent) {

    Node *node = malloc(sizeof(Node));
    node->ty = ND_LVAR;
    node = init_ident(parent, node);
    
    if (node == NULL)
        return NULL;

    if (consume('(')) {
        pos--;
        return NULL;
    }
    
    printf("=======================\n");
    printf("var address: %d\n", node);
    printf("var name: %s\n", node->name);
    printf("var parent: %d\n", node->parent);
    printf("var offset: %d\n", node->offset);

    return node;
}

Node *parse_func(Node *parent) {
    Node *node = malloc(sizeof(Node));
    node = init_ident(parent, node);

    if (node == NULL)
        return NULL;

    if (!consume('(')) {
        pos--;
        return NULL;
    }

    node->args = new_vector();
    node->ty = ND_FUNC;

    while (!consume(')')) {
        vec_push(node->args, expr(node));
        consume(',');
    }

    if (map_exists(parent->funcs, node->name) == 0 && map_exists(global_node->funcs, node->name) == 0) {
        //error
        printf("ERROR NOT EXIST FUNCTION\n");
        return NULL;
    }

    return node;
}

Node *parse_num(Node *parent) {
    if (!consume(TK_NUM))
        return NULL;

    Node *node = malloc(sizeof(Node));
    Token *t = tokens->data[pos-1];
    node->ty = ND_NUM;
    node->val = t->val;

    return node;
}

Node *parse_if(Node *parent) {
    if (!consume(TK_IF))
        return NULL;

    if (!consume('(')) {
        printf("ERROR if文の(");
        return NULL;
    }

    Node *node = malloc(sizeof(Node));
    node->ty = ND_IF;
    node->then = new_vector();
    node->idents = new_map();
    node->funcs = new_map();
    node->offset = 0;

    node->cond = expr(node);

    if (!consume(')')) {
        printf("ERROR if文の)");
        return NULL;
    }

    if (!consume('{')) {
        printf("ERROR if文の{");
        return NULL;
    }

    while (!consume('}')) {
        vec_push(node->then, stmt(node));
    }

    if (consume(TK_ELSE))
        return node->els = stmt(node);

    return node;
}

void program() {
    int i = 0;
    global_node = malloc(sizeof(Node));
    global_node->idents = new_map();
    global_node->funcs = new_map();
    while (!consume(TK_EOF)) {
        code[i++] = parse_decl(global_node);
    }

    code[i] = NULL;

    return;
}

Node *stmt(Node *parent) {
    Node *node;

    if ((node = parse_decl(parent)) != NULL)
        return node;

    if ((node = parse_return(parent)) != NULL)
        return node;

    if ((node = parse_if(parent)) != NULL)
        return node;
    
    if (consume(TK_WHILE)) {
        if (!consume('(')) {
            Token *t = tokens->data[pos];
            error_at(t->input, "while文の'('がありません");
        }

        node = malloc(sizeof(Node));
        node->ty = ND_WHILE;
        node->cond = expr(parent);

        if (!consume(')')) {
            Token *t = tokens->data[pos];
            error_at(t->input, "while文の')'がありません");
        }

        node->loop = stmt(parent);

        return node;
    }

    if (consume(TK_FOR)) {
        if (!consume('(')) {
            Token *t = tokens->data[pos];
            error_at(t->input, "for文の'('がありません");
        }

        node = malloc(sizeof(Node));
        node->ty = ND_FOR;

        if (!consume(';')) {
            node->init = expr(parent);
            if (!consume(';')) {
                Token *t = tokens->data[pos];
                error_at(t->input, "for文の';'がありません");
            }
        }
    
        if (!consume(';')) {
            node->cond = expr(parent);
            if (!consume(';')) {
                Token *t = tokens->data[pos];
                error_at(t->input, "for文の';'がありません");
            }
        }

        if (!consume(')')) {
            node->inc = expr(parent);
            if (!consume(')')) {
                Token *t = tokens->data[pos];
                error_at(t->input, "for文の')'がありません");
            }
        }

        node->loop = stmt(parent);

        return node;
    }

    if (consume('{')) {
        node = malloc(sizeof(Node));
        node->ty = ND_BLOCK;
        node->block = new_vector();

        while (!consume('}')) {
            vec_push(node->block, stmt(node));
        }

        return node;
    }

    node = expr(parent);

    if (node->ty != ND_DFUNC && !consume(';')) {
        Token *t = tokens->data[pos];
        error_at(t->input, "';'ではないトークンです");
    }

    return node;
}

Node *expr(Node *parent) {
    return assign(parent);
}

Node *assign(Node *parent) {
    Node *node = equality(parent);

    if (consume('='))
        node = new_node('=', node, assign(parent));
    return node;
}

Node *equality(Node *parent) {
    Node *node = relational(parent);

    for (;;) {
        if (consume(TK_EQ))
            node = new_node(ND_EQ, node, relational(parent));
        else if (consume(TK_NE))
            node = new_node(ND_NE, node, relational(parent));
        else
            return node;
    }
}

Node *relational(Node *parent) {
    Node *node = add(parent);

    for (;;) {
        if (consume('<'))
            node = new_node('<', node, add(parent));
        else if (consume('>'))
            node = new_node('>', node, add(parent));
        else if (consume(TK_LE))
            node = new_node(ND_LE, node, add(parent));
        else if (consume(TK_GE))
            node = new_node(ND_GE, node, add(parent));
        else
            return node;
    }

    return node;
}


Node *add(Node *parent) {
    Node *node = mul(parent);

    for (;;) {
        if (consume('+'))
            node = new_node('+', node, mul(parent));
        else if (consume('-'))
            node = new_node('-', node, mul(parent));
        else
            return node;
    }
}

Node *mul(Node *parent) {
    Node *node = unary(parent);

    for (;;) {
        if (consume('*')) {
            node = new_node('*', node, unary(parent));
         } else if (consume('/'))
            node = new_node('/', node, unary(parent));
        else
            return node;
    }
}

Node *unary(Node *parent) {
    if (consume(TK_SIZEOF)) {
        Node *node = malloc(sizeof(Node));
        node->ty = ND_SIZEOF;
        Type *t = malloc(sizeof(Type));
        t->ty = INT;
        node->type = t;
        node->lhs = unary(parent);
        return node;
    }
    if (consume('+'))
        return term(parent);
    if (consume('-'))
        return new_node('-', new_node_num(0), term(parent));
    return term(parent);
}

Node *term(Node *parent) {
    Token *t = tokens->data[pos];
    Node *node = malloc(sizeof(Node));

    if ((node = parse_func(parent)) != NULL) {
        return node;
    }

    if ((node = parse_num(parent)) != NULL) {
        return node;
    }

    if ((node = parse_lvar(parent)) != NULL) {
        return node;
    }

    if (consume('*')) {
        Node *node = malloc(sizeof(Node));
        node->token = t;
        node->ty = ND_DEREF;
        node->lhs = term(parent);
        return node;
    }

    if (consume('&')) {
        Node *node = malloc(sizeof(Node));
        node->token = t;
        node->ty = ND_ADDR;
        node->lhs = term(parent);
        return node;
    }
    
    printf("%d\n", t->ty);
    error_at(t->input, "数値でも開き括弧でもないトークンです");
    exit(1);
}
