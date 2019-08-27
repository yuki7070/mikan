#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mikan.h"

int pos = 0;
int str = 0;

Node *function();
Node *stmt(Node *parent);
Node *expr(Node *parent);
Node *assign(Node *parent);
Node *equality(Node *parent);
Node *logic(Node *parent);
Node *relational(Node *parent);
Node *add(Node *parent);
Node *mul(Node *parent);
Node *term(Node *parent);
Node *unary(Node *parent);

Node *init_func_arg(Node *parent);

int new_func(Node *parent, Node *node);

int get_size(Node *node) {
    int s = 0;
    switch (node->type->ty) {
        case TY_INT:
            return 4;
            break;
        case TY_CHAR:
            return 1;
            break;
        case TY_PTR:
            return 8;
            break;
        case TY_VOID:
            return 0;
            break;
        case TY_ARRAY:
            switch (node->type->ptr_to->ty) {
            case TY_CHAR:
                s = 1;
                break;
            case TY_INT:
                s = 4;
                break;
            case TY_PTR:
                s = 8;
                break;
            case TY_VOID:
                s = 0;
                break;
            }
            return node->type->array_size * s;
            break;
        default:
            return -1;
    }
}

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

Node *conv_arr_to_ptr(Node *lhs) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_DEREF;
    node->lhs = new_node('+', lhs, new_node_num(lhs->type->array_size));
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

    if (!consume('{')) {
        printf("error\n");
        return 1;
    }

    map_put(parent->funcs, node->name, node);

    while (!consume('}')) {
        vec_push(node->block, stmt(node));
    }

    return 1;
}

void calc_offset(Node *parent, int size) {
    if (parent->parent) {
        parent->offset += parent->parent->offset + size;
        return calc_offset(parent->parent, size);
    }
    parent->offset += size;
    return;
}

int new_decl_var(Node *parent, Node *node) {
    int size = get_size(node);
    node->type->size = size;

    if (parent->ty != ND_GNODE) {
        calc_offset(parent, size);
        node->offset = parent->offset;
    } else {
        node->offset = size;
    }
    
    map_put(parent->idents, node->name, node);

    if (!consume(';'))
        return 0;
    
    return 1;
}

Node *init_type() {
    int ty;

    if (consume(TK_INT)) {
        ty = TY_INT;
    } else if (consume(TK_CHAR)) {
        ty = TY_CHAR;
    } else if (consume(TK_VOID)) {
        ty = TY_VOID;
    } else {
        //未定義の型
        return NULL;
    }

    Node *node = malloc(sizeof(Node));

    if (consume('*')) {
        Type *t = malloc(sizeof(Type));
        t->ty = TY_PTR;
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

Node *init_func_ident(Node *parent, Node *node) {
    if (!consume(TK_IDENT)) {
        return NULL;
    }

    Token *t = tokens->data[pos-1];
    node->name = t->name;
    node->token = t;
    node->parent = parent;

    return node;
}

Node *init_ident(Node *parent, Node *node) {
    if (!consume(TK_IDENT)) {
        return NULL;
    }

    Token *t = tokens->data[pos-1];
    node->name = t->name;
    node->token = t;
    node->parent = parent;

    if (!consume('[')) {
        return node;
    }

    //配列である場合
    Type *ty_1 = node->type;
    Type *ty_2 = malloc(sizeof(Type));
    ty_2->ty = TY_ARRAY;
    ty_2->ptr_to = ty_1;
    node->type = ty_2;
    if (!consume(TK_NUM)) {
        printf("ERROR\n");
        return NULL;
    }
    
    Token *t_1 = tokens->data[pos-1];
    node->type->array_size = t_1->val;

    if (!consume(']')) {
        printf("ERROR\n");
        return NULL;
    }

    return node;
}

Node *init_func_arg(Node *parent) {

    if (!consume(TK_TYPE))
        return NULL;
    Node *node = init_type();
    node = init_func_ident(parent, node);

    node->ty = ND_DVAR;
    int size = 0;
    switch (node->type->ty) {
        case TY_INT:
            size = 8;
            break;
        case TY_PTR:
            size = 8;
            break;
        case TY_VOID:
            size = 0;
            break;
        case TY_CHAR:
            size = 1;
            break;
        case TY_ARRAY:
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
        if (parent->ty == ND_GNODE) {
            node->ty = ND_GVAR;
        } else {
            node->ty = ND_LVAR;
        }
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
    
    if (node == NULL) {
        return NULL;
    }

    if (node->type)
        return conv_arr_to_ptr(node);
    
    return node;
}

Node *parse_func(Node *parent) {
    Node *node = malloc(sizeof(Node));
    node = init_func_ident(parent, node);

    if (node == NULL)
        return NULL;

    if (!consume('(')) {
        pos--;
        return NULL;
    }

    node->args = new_vector();
    node->ty = ND_FUNC;

    while (!consume(')')) {
        vec_push(node->args, expr(parent));
        consume(',');
    }

    return node;
}

Node *parse_num(Node *parent) {
    if (!consume(TK_NUM))
        return NULL;

    Token *t = tokens->data[pos-1];
    Node *node = new_node_num(t->val);

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
    node->parent = parent;
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

    if (!consume(TK_ELSE))
        return node;

    node->els = new_vector();

    if (!consume('{')) {
        printf("ERROR if文の{");
        return NULL;
    }

    while (!consume('}')) {
        vec_push(node->els, stmt(node));
    }
    
    return node;
}

Node *parse_for(Node *parent) {
    if (!consume(TK_FOR))
        return NULL;

    Node *node = malloc(sizeof(Node));
    node->ty = ND_FOR;
    node->loop = new_vector();
    node->idents = new_map();
    node->funcs = new_map();
    node->parent = parent;
    node->offset = 0;
    
    if (!consume('(')) {
        printf("ERROR for文の(");
        exit(1);
    }

    if (!consume(';')) {
        node->init = stmt(node);
    }

    if (!consume(';')) {
        node->cond = stmt(node);
    }

    if (!consume(')')) {
        node->inc = expr(node);
        if (!consume(')')) {
            printf("ERROR");
            exit(1);
        }
    }

    if (!consume('{')) {
        printf("ERROR for文の{");
        exit(1);
    }

    while (!consume('}')) {
        vec_push(node->loop, stmt(node));
    }

    return node;
}

Node *parse_while(Node *parent) {
    if (!consume(TK_WHILE))
        return NULL;

    if (!consume('(')) {
        printf("ERROR while文の(");
        exit(1);
    }

    Node *node = malloc(sizeof(Node));
    node->ty = ND_WHILE;
    node->loop = new_vector();
    node->idents = new_map();
    node->funcs = new_map();
    node->parent = parent;
    node->offset = 0;

    if (!consume(')')) {
        node->cond = expr(node);
        if (!consume(')')) {
            printf("ERROR");
            exit(1);
        }
    }

    if (!consume('{')) {
        printf("ERROR while文の{");
        exit(1);
    }

    while (!consume('}')) {
        vec_push(node->loop, stmt(node));
    }
    
    return node;
}

Node *parse_indirection(Node *parent) {
    if (!consume('*'))
        return NULL;

    Node *node = malloc(sizeof(Node));
    node->ty = ND_DEREF;
    
    node->lhs = term(parent);
    if (node->lhs == NULL)
        return NULL;

    return node;
}

Node *parse_address(Node *parent) {
    if (!consume('&'))
        return NULL;

    Node *node = malloc(sizeof(Node));
    node->ty = ND_ADDR;

    node->lhs = term(parent);
    if (node->lhs == NULL)
        return NULL;

    return node;
}

Node *parse_sizeof(Node *parent) {
    if (!consume(TK_SIZEOF))
        return NULL;
    
    Node *node = malloc(sizeof(Node));
    node->ty = ND_SIZEOF;
    node->parent = parent;
    Type *t = malloc(sizeof(Type));
    t->ty = TY_INT;
    node->type = t;

    node->lhs = unary(parent);
    return node;
}

Node *parse_char_literal(Node *parent) {
    if (!consume(TK_STR))
        return NULL;

    Node *node = malloc(sizeof(Node));
    node->ty = ND_STR;
    Token *t = tokens->data[pos-1];
    node->name = t->name;
    node->offset = str++;
    vec_push(char_literals, node);
    return node;
}

Node *parse_break(Node *parent) {
    if (!consume(TK_BREAK))
        return NULL;

    Node *node = malloc(sizeof(Node));
    node->ty = ND_BREAK;
    node->parent = parent;
    if (!consume(';')) {
        Token *t = tokens->data[pos];
        error_at(t->input, "';'ではないトークンです");
    }
    return node;
}

Node *parse_continue(Node *parent) {
    if (!consume(TK_CONTINUE))
        return NULL;

    Node *node = malloc(sizeof(Node));
    node->ty = ND_CONTINUE;
    node->parent = parent;
    if (!consume(';')) {
        Token *t = tokens->data[pos];
        error_at(t->input, "';'ではないトークンです");
    }
    return node;
}

Node *parse_switch(Node *parent) {
    if (!consume(TK_SWITCH))
        return NULL;

    if (!consume('(')) {
        Token *t = tokens->data[pos];
        error_at(t->input, "'('ではないトークンです");
    }

    Node *node = malloc(sizeof(Node));
    node->ty = ND_SWITCH;
    node->idents = new_map();
    node->funcs = new_map();
    node->then = new_vector();
    node->cases = new_vector();
    node->block = new_vector();
    node->parent = parent;
    node->offset = 0;

    node->cond = expr(node);

    if (!consume(')')) {
        Token *t = tokens->data[pos];
        error_at(t->input, "')'ではないトークンです");
    }

    if (!consume('{')) {
        Token *t = tokens->data[pos];
        error_at(t->input, "'{'ではないトークンです");
    }

    int first = 1;
    int end = 0;

    while (!consume('}')) {
        if (first == 1 && !consume(TK_CASE)) {
            while (!consume(TK_CASE)) {
                vec_push(node->then, stmt(node));
            }
        }
        first = 0;
        vec_push(node->cases, expr(node));
        if (!consume(':')) {
            Token *t = tokens->data[pos];
            error_at(t->input, "':'ではないトークンです");
        }

        Vector *vec = new_vector();

        while (!consume(TK_CASE)) {
            vec_push(vec, stmt(node));
            if (consume('}')) {
                end = 1;
                break;
            }
        }

        vec_push(node->block, vec);
        
        if (end)
            break;
    }

    return node;
}

void program() {
    int i = 0;
    global_node = malloc(sizeof(Node));
    global_node->idents = new_map();
    global_node->funcs = new_map();
    global_node->ty = ND_GNODE;
    while (!consume(TK_EOF)) {
        code[i++] = parse_decl(global_node);
    }

    code[i] = NULL;

    return;
}

Node *stmt(Node *parent) {
    Node *node;

    if ((node = parse_decl(parent)) != NULL) {
        return node;
    }

    if ((node = parse_return(parent)) != NULL)
        return node;

    if ((node = parse_if(parent)) != NULL)
        return node;
    
    if ((node = parse_while(parent)) != NULL)
        return node;

    if ((node = parse_for(parent)) != NULL)
        return node;

    if ((node = parse_break(parent)) != NULL)
        return node;

    if ((node = parse_continue(parent)) != NULL)
        return node;

    if ((node = parse_switch(parent)) != NULL)
        return node;

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

    
    if (!consume(';')) {
        Token *t = tokens->data[pos];
        printf("type: %d\n", node->ty);
        error_at(t->input, "';'ではないトークンです");
    }

    return node;
}

Node *expr(Node *parent) {
    return assign(parent);
}

Node *assign(Node *parent) {
    Node *node = logic(parent);

    if (consume('=')) {
        node = new_node('=', node, assign(parent));
    }
        
    return node;
}

Node *logic(Node *parent) {
    Node *node = equality(parent);

    if (consume(TK_AND))
        node = new_node(ND_AND, node, logic(parent));
    else if (consume(TK_OR))
        node = new_node(ND_OR, node, logic(parent));
    else
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
    Node *node = malloc(sizeof(Node));

    if ((node = parse_sizeof(parent)) != NULL) {
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

    if (consume('(')) {
        Node *node = expr(parent);
        if (!consume(')')) {
            error_at(t->input, "開き括弧に対応する閉じ括弧がありません");
        }
        return node;
    }

    if ((node = parse_func(parent)) != NULL) {
        return node;
    }

    if ((node = parse_num(parent)) != NULL) {
        return node;
    }

    if ((node = parse_lvar(parent)) != NULL) {
        return node;
    }

    if ((node = parse_indirection(parent)) != NULL) {
        return node;
    }

    if ((node = parse_address(parent)) != NULL) {
        return node;
    }

    if ((node = parse_char_literal(parent)) != NULL) {
        return node;
    }

    return;
}
