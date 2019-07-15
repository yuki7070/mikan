#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mikan.h"

int pos = 0;

Node *function();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *term();
Node *unary();

Node *lval(int ty, Node *n);

int *parse_func(Node *parent, Node *node);

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

Node *new_node_ident(int offset) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_LVAR;
    node->offset = offset;
    return node;
}

Node *parse_ident() {
    int ty;

    if (consume(TK_INT)) {
        ty = INT;
    } else {
        //未定義の型
    }

    Node *node = malloc(sizeof(Node));

    if (consume('*')) {
        Type *t = malloc(sizeof(Type));
        t->ty = PTR;
        node->type = t;
    }

    Type *t = malloc(sizeof(Type));
    t->ty = ty;

    if (node->ty == PTR) {
        node->type->ptr_to = t;
    } else {
        node->type = t;
    }

    Token *token = tokens->data[pos];
    node->name = token->name;
    node->token = token;

    return node;
}

Node *parse_decl(Node *parent) {
    if (!consume(TK_TYPE))
        return NULL;

    Node *node = parse_ident();

    if (parse_func(parent, node) == 1)
        return node;
    
    
}

int *parse_func(Node *parent, Node *node) {
    if (!consume('('))
        return 0;

    node->args = new_vector();
    node->ty = ND_DFUNC;
    node->block = new_vector();
    node->idents = new_map();
    node->funcs = new_map();
    node->offset = 0;

    while (!consume(')')) {
        vec_push(node->args, expr(node));
        consume(',');
    }

    Vector *args = node->args;
    for (int j = 0; j < args->len; j++) {
        Node *arg = args->data[j];
        map_put(node->idents, arg->name, arg);
        node->offset = arg->offset;
    }

    if (!consume('{'))
        return 0;

    while (consume('}')) {
        vec_push(node->block, stmt(node));
    }

    map_put(parent->funcs, node->name, node);

    return 1;
    
}

void program() {
    int i = 0;
    while (!consume(TK_EOF)) {
        if (consume(TK_TYPE)) {
            if (consume(TK_INT)) {
                Node *node = lval(INT, NULL);
                Token *t = tokens->data[pos];
                pos++;
                node->name = t->name;
                node->token = t;

                if (consume('(')) {
                    node->args = new_vector();

                    while (!consume(')')) {
                        vec_push(node->args, expr());
                        consume(',');
                    }

                    if (consume('{')) {
                        node->ty = ND_DFUNC;
                        //printf("TESTEARE%d\n", node->ty);
                        node->block = new_vector();
                        Map *func_ident = new_map();

                        map_put(functions, node->name, node);

                        Vector *args = node->args;

                        for (int j = 0; j < args->len; j++) {
                            Node *n = args->data[j];
                            map_put(func_ident, n->name, (j+1)*8);
                        }

                        while (!consume('}')) {
                            vec_push(node->block, stmt());
                        }
                    }

                    code[i++] = node;
                    //printf("TEST%d\n", i);
                    continue;
                };

                if (consume('[')) {
                    node->type->ty = ARRAY;
                    Type *t_a = malloc(sizeof(Type));
                    t_a->ty = INT;
                    node->type->ptr_to = t_a;
                    if (!consume(TK_NUM))
                        error_at(t->input, "配列のサイズ");
                    Token *t = tokens->data[pos-1];
                    node->type->array_size = t->val+1;
                    if (consume(']'))
                        error_at(t->input, "配列のとじカッコ");
                }

                node->ty = ND_DVAR;
                code[i++] = node;
                continue;
            }
        }

        error("%s", "なんかあれ");
    }

    code[i] = NULL;

    return;
}

Node *stmt() {
    Node *node;

    if (consume(TK_RETURN)) {
        node = malloc(sizeof(Node));
        node->ty = ND_RETURN;
        node->lhs = expr();

        if (!consume(';')) {
            Token *t = tokens->data[pos];
            error_at(t->input, "';'ではないトークンです");
        }

        return node;
    }

    if (consume(TK_IF)) {
        if (!consume('(')) {
            Token *t = tokens->data[pos];
            error_at(t->input, "if文の'('がありません");
        }

        node = malloc(sizeof(Node));
        node->ty = ND_IF;
        node->cond = expr();

        if (!consume(')')) {
            Token *t = tokens->data[pos];
            error_at(t->input, "if文の')'がありません");
        }

        node->then = stmt();

        if (consume(TK_ELSE))
            node->els = stmt();

        return node;
    }
    
    if (consume(TK_WHILE)) {
        if (!consume('(')) {
            Token *t = tokens->data[pos];
            error_at(t->input, "while文の'('がありません");
        }

        node = malloc(sizeof(Node));
        node->ty = ND_WHILE;
        node->cond = expr();

        if (!consume(')')) {
            Token *t = tokens->data[pos];
            error_at(t->input, "while文の')'がありません");
        }

        node->loop = stmt();

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
            node->init = expr();
            if (!consume(';')) {
                Token *t = tokens->data[pos];
                error_at(t->input, "for文の';'がありません");
            }
        }
    
        if (!consume(';')) {
            node->cond = expr();
            if (!consume(';')) {
                Token *t = tokens->data[pos];
                error_at(t->input, "for文の';'がありません");
            }
        }

        if (!consume(')')) {
            node->inc = expr();
            if (!consume(')')) {
                Token *t = tokens->data[pos];
                error_at(t->input, "for文の')'がありません");
            }
        }

        node->loop = stmt();

        return node;
    }

    if (consume('{')) {
        node = malloc(sizeof(Node));
        node->ty = ND_BLOCK;
        node->block = new_vector();

        while (!consume('}')) {
            vec_push(node->block, stmt());
        }

        return node;
    }

    node = expr();

    if (node->ty != ND_DFUNC && !consume(';')) {
        Token *t = tokens->data[pos];
        error_at(t->input, "';'ではないトークンです");
    }

    return node;
}

Node *expr() {
    return assign();
}

Node *assign() {
    Node *node = equality();

    if (consume('='))
        node = new_node('=', node, assign());
    return node;
}

Node *equality() {
    Node *node = relational();

    for (;;) {
        if (consume(TK_EQ))
            node = new_node(ND_EQ, node, relational());
        else if (consume(TK_NE))
            node = new_node(ND_NE, node, relational());
        else
            return node;
    }
}

Node *relational() {
    Node *node = add();

    for (;;) {
        if (consume('<'))
            node = new_node('<', node, add());
        else if (consume('>'))
            node = new_node('>', node, add());
        else if (consume(TK_LE))
            node = new_node(ND_LE, node, add());
        else if (consume(TK_GE))
            node = new_node(ND_GE, node, add());
        else
            return node;
    }

    return node;
}


Node *add() {
    Node *node = mul();

    for (;;) {
        if (consume('+'))
            node = new_node('+', node, mul());
        else if (consume('-'))
            node = new_node('-', node, mul());
        else
            return node;
    }
}

Node *mul() {
    Node *node = unary();

    for (;;) {
        if (consume('*')) {
            node = new_node('*', node, unary());
         } else if (consume('/'))
            node = new_node('/', node, unary());
        else
            return node;
    }
}

Node *unary() {
    if (consume(TK_SIZEOF)) {
        Node *node = malloc(sizeof(Node));
        node->ty = ND_SIZEOF;
        Type *t = malloc(sizeof(Type));
        t->ty = INT;
        node->type = t;
        node->lhs = unary();
        return node;
    }
    if (consume('+'))
        return term();
    if (consume('-'))
        return new_node('-', new_node_num(0), term());
    return term();
}

Node *term() {
    Token *t = tokens->data[pos];

    if (consume('(')) {
        Node *node = expr();
        if (!consume(')')) {
            error_at(t->input, "開き括弧に対応する閉じ括弧がありません");
        }
        return node;
    }

    if (consume(TK_NUM)) {
        return new_node_num(t->val);
    }

    if (consume(TK_TYPE)) {
        
        if (consume(TK_INT)) {
            Node *node = lval(INT, NULL);
            Token *t = tokens->data[pos];
            pos++;

            node->name = t->name;
            node->token = t;

            if (consume('(')) {
               node->args = new_vector(); 

               while (!consume(')')) {
                    vec_push(node->args, expr());
                    consume(',');
                }

                if (consume('{')) {
                    node->ty = ND_DFUNC;
                    node->block = new_vector();
                    Map *func_ident = new_map();

                    map_put(functions, node->name, node);

                    Vector *args = node->args;

                    for (int j = 0; j < args->len; j++) {
                        Node *n = args->data[j];
                        map_put(func_ident, n->name, (j+1)*8);
                    }

                    while (!consume('}')) {
                        vec_push(node->block, stmt());
                    }
                } else {
                    node->ty = ND_FUNC;
                }

                return node;
            }

            if (consume('[')) {
                node->type->ty = ARRAY;
                Type *t_a = malloc(sizeof(Type));
                t_a->ty = INT;
                node->type->ptr_to = t_a;
                if (!consume(TK_NUM))
                    error_at(t->input, "配列のサイズがぁぁぁあ？？？");
                Token *t = tokens->data[pos-1];
                node->type->array_size = t->val + 1;
                if (!consume(']'))
                    error_at(t->input, "配列のとじ括弧がぁぁ？？？");
            }

            node->ty = ND_DVAR;

            return node;
        }

        error_at(t->input, "不明な型です");
    }

    if (consume(TK_IDENT)) {

        Node *node = malloc(sizeof(Node));
        node->token = t;
        node->name = t->name;

        if (consume('(')) {
            node->args = new_vector();

            while (!consume(')')) {
                vec_push(node->args, expr());
                consume(',');
            }
            
            if (consume('{')) {
                error_at(t->input, "型が定義されていない関数宣言です");
            }
            
            node->ty = ND_FUNC;

            return node;
        }

        node->ty = ND_LVAR;

        if (consume('[')) {

            Node *dererf = malloc(sizeof(Node));
            dererf->token = t;
            dererf->ty = ND_DEREF;
            dererf->lhs = new_node('+', node, expr());

            if (!consume(']'))
                error_at(t->input, "配列のとじ括弧がぁぁ？？？");
            
            return dererf;
            
        }

        return node;
    }

    if (consume('*')) {
        Node *node = malloc(sizeof(Node));
        node->token = t;
        node->ty = ND_DEREF;
        node->lhs = term();
        return node;
    }

    if (consume('&')) {
        Node *node = malloc(sizeof(Node));
        node->token = t;
        node->ty = ND_ADDR;
        node->lhs = term();
        return node;
    }
    
    error_at(t->input, "数値でも開き括弧でもないトークンです");
    exit(1);
}

Node *lval(int ty, Node *n) {
    Node *node = malloc(sizeof(Node));

    if (n != NULL) {
        node = n;
    }

    if (read_ahead(TK_IDENT)) {
        Type *t = malloc(sizeof(Type));
        t->ty = ty;
        if (n != NULL) {
            node->type->ptr_to = t;
        } else {
            node->type = t;
        }
        return node;
    }

    if (consume('*')) {
        Type *t = malloc(sizeof(Type));
        t->ty = PTR;
        node->type = t;
        return lval(ty, node);
    }

    Token *t = tokens->data[pos];
    error_at(t->input, "不明な型でしゅ");
    exit(1);
}

Node *typed_val(int ty, Node *n) {
    Node *node = malloc(sizeof(Node));

    if (n != NULL) {
        node = n;
    }

    Type *t = malloc(sizeof(Type));
    t->ty = ty;
    node->type = t;
}