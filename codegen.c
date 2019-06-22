#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mikan.h"

int pos = 0;

int jump_count = 0;

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

int consume(int ty) {
    Token *t = tokens->data[pos];

    if (t->ty != ty)
        return 0;
    pos++;
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

void program() {
    int i = 0;
    while (!consume(TK_EOF)) {
        code[i++] = stmt();
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
        if (consume('*'))
            node = new_node('*', node, unary());
        else if (consume('/'))
            node = new_node('/', node, unary());
        else
            return node;
    }
}

Node *unary() {
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

    if (consume(TK_IDENT)) {
        
        if (consume('(')) {
            Node *node = malloc(sizeof(Node));
            node->name = t->name;
            node->args = new_vector();
            int ident_count = 0;

            while (!consume(')')) {
                vec_push(node->args, term());
                consume(',');
            }
            
            if (consume('{')) {
                node->ty = ND_DFUNC;
                node->name = t->name;
                node->block = new_vector();
                Map *func_ident = new_map();
                
                map_put(functions, node->name, node);

                Vector *args = node->args;

                for (int j = 0; j < args->len; j++) {
                    Node *n = args->data[j];
                    map_put(func_ident, n->name, (j+1)*8);
                }

                while (!consume('}')) {
                    //Node *n = stmt();
                    vec_push(node->block, stmt());
                    //printf("%d\n", n->ty);
                }
            } else {
                node->ty = ND_FUNC;
            }

           /*

            if (!consume('{'))
                return node;

            node->func = stmt();

            */

            return node;
        }

        Node *node = malloc(sizeof(Node));
        node->ty = ND_LVAR;
        node->name = t->name;

        return node;
    }
    
    error_at(t->input, "数値でも開き括弧でもないトークンです");
    exit(1);
}

void func_lval(Node *parent, Node *node) {

    //printf("%d\n", node->ty);

    //printf("%d\n", node->ty);
    if (node->ty != ND_LVAR) {
        node->parent = malloc(sizeof(parent));
        node->parent = parent;
        if (node->ty == ND_RETURN)
            parent->is_return = 1;
        if (node->lhs)
            func_lval(parent, node->lhs);
        if (node->rhs)
            func_lval(parent, node->rhs);
        if (node->ty == ND_IF) {
            node->then->parent = parent;
            if (node->cond)
                node->cond->parent = parent;
            if (node->els)
                node->els->parent = parent;
        }
        if (node->ty == ND_FUNC) {
            Vector *args = node->args;
            for (int j = 0; j < args->len; j++) {
                func_lval(parent, args->data[j]);
            }
        }
        return;
    }

    Map *idents = parent->idents;
    Map *parent_idents = NULL;

    if (parent->parent != NULL && (parent->parent->ty == ND_DFUNC || parent->parent->ty == ND_BLOCK)) {
        parent_idents = parent->parent->idents;
    }
    int offset = 0;


    if (parent_idents != NULL && map_exists(parent_idents, node->name)) {
        int parent_offset = (int)map_get(parent_idents, node->name);
        offset = parent_offset - (parent_idents->keys->len+1)*8;
    } else if (map_exists(idents, node->name) == 1) {
        offset = (int)map_get(idents, node->name);
    } else {
        offset = (idents->keys->len + 1) * 8;
        map_put(idents, node->name, offset);
    }

    node->offset = offset;
    return;
}

void gen_lval(Node *node) {
    if (node->ty != ND_LVAR)
        error("代入の左辺値が変数ではありません");

    printf("    mov rax, rbp\n");
    if (node->offset < 0) {
        printf("    add rax, %d\n", node->offset*(-1));
    } else {
        printf("    sub rax, %d\n", node->offset);
    }
    printf("    push rax\n");
}

void gen(Node *node) {
    if (node->ty == ND_RETURN) {
        gen(node->lhs);
        printf("    pop rax\n");
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        return;
    }

    if (node->ty == ND_IF) {
        int j1 = jump_count++;
        int j2 = jump_count++;
        Map *idents = new_map();
        node->idents = idents;
        func_lval(node, node->cond);
        Vector *block = node->then->block;
        for (int j = 0; j < block->len; j++) {
            func_lval(node, block->data[j]);
        }
        if (node->els) {
            Vector *block = node->els->block;
            for (int j = 0; j < block->len; j++) {
                func_lval(node, block->data[j]);
            }
        }

        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, %d\n", idents->keys->len*8);
        
        gen(node->cond);

        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        if (node->els) {
            printf("    je  .Lelse%d\n", j1);
            gen(node->then);
            printf("    jmp  .Lend%d\n", j2);
            printf(".Lelse%d:\n", j1);
            gen(node->els);
        } else {
            printf("    je  .Lend%d\n", j2);
            gen(node->then);
        }
        printf(".Lend%d:\n", j2);
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");

        return;
    }

    if (node->ty == ND_WHILE) {
        int j1 = jump_count++;
        int j2 = jump_count++;
        printf(".Lbegin%d:\n", j1);
        gen(node->cond);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je  .Lend%d\n", j2);
        gen(node->loop);
        printf("    jmp .Lbegin%d\n", j1);
        printf(".Lend%d:\n", j2);
        return;
    }

    if (node->ty == ND_FOR) {
        if (node->init) {
            gen(node->init);
        }
        int j1 = jump_count++;
        int j2 = jump_count++;
        printf(".Lbegin%d:\n", j1);
        if (node->cond) {
            gen(node->cond);
            printf("    pop rax\n");
            printf("    cmp rax, 0\n");
            printf("    je  .Lend%d\n", j2);
        }
        gen(node->loop);

        if (node->inc) {
            gen(node->inc);
        }
        printf("    jmp .Lbegin%d\n", j1);
        printf(".Lend%d:\n", j2);
        return;
    }

    if (node->ty == ND_BLOCK) {
        Vector *block = node->block;
        
        Map *idents = new_map();
        node->idents = idents;
/*
        for (int j = 0; j < block->len; j++) {
            func_lval(node, block->data[j]);
        }
*/      
        /*
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, %d\n", idents->keys->len*8);
        */
        
        for (int j = 0; j < block->len; j++) {
            Node *n = block->data[j];
            gen(n);
            if (n->ty != ND_RETURN) {
                printf("    pop rax\n");
            }
        }

        /*
        if (node->is_return != 1) {
            printf("    mov rsp, rbp\n");
            printf("    pop rbp\n");   
        }
        */

        return;
    }

    if (node->ty == ND_FUNC) {
        Vector *args = node->args;
        /*
        Map *idents = new_map();
        node->idents = idents;
        for (int j = 0; j < args->len; j++) {
            Node *n = args->data[j];
            printf("%d\n", n->ty);
            func_lval(node, args->data[j]);
        }
        */
        
        for (int j = 0; j < args->len; j++) {
            gen(args->data[j]);
        }
        for (int j = args->len - 1; j >= 0; j--) {
            printf("    pop rax\n");
            switch (j) {
            case 0:
                printf("    mov rdi, rax\n");
                break;
            case 1:
                printf("    mov rsi, rax\n");
                break;
            case 2:
                printf("    mov rdx, rax\n");
                break;
            case 3:
                printf("    mov rcx, rax\n");
                break;
            case 4:
                printf("    mov r8, rax\n");
                break;
            case 5:
                printf("    mov r9, rax\n");
                break;
            }
        }
        printf("    call %s\n", node->name);
        printf("    push rax\n");
        return;
    }

    if (node->ty == ND_DFUNC) {
        Vector *args = node->args;
        Vector *block = node->block;
        Map *idents = new_map();
        node->idents = idents;

        //printf("AREARE\n");
        for (int j = 0; j < args->len; j++) {
            func_lval(node, args->data[j]);
        }
        
        for (int j = 0; j < block->len; j++) {
            func_lval(node, block->data[j]);
        }
        printf("%s: \n", node->name);
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, %d\n", idents->keys->len*8);

        for (int j = 0; j < args->len; j++) {
            Node *n = args->data[j];
            //printf("%d\n", n->ty);
            gen_lval(n);
            switch (j) {
            case 0:
                printf("    push rdi\n");
                break;
            case 1:
                printf("    push rsi\n");
                break;
            case 3:
                printf("    push rdx\n");
                break;
            }
            printf("    pop rdi\n");
            printf("    pop rax\n");
            printf("    mov [rax], rdi\n");
        }
        
        for (int j = 0; j < block->len; j++) {
            Node *n = block->data[j];
            gen(n);
            if (n->ty != ND_IF && n->ty != ND_RETURN) {
                printf("    pop rax\n");
            }
        }

        if (node->is_return != 1) {
            printf("    mov rsp, rbp\n");
            printf("    pop rbp\n");
            printf("    ret\n");
        }
        
        return;
    }

    if (node->ty == ND_NUM) {
        printf("    push %d\n", node->val);
        return;
    }

    if (node->ty == ND_LVAR) {
        gen_lval(node);
        printf("    pop rax\n");
        printf("    mov rax, [rax]\n");
        printf("    push rax\n");
        return;
    }

    if (node->ty == '=') {
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("    pop rdi\n");
        printf("    pop rax\n");
        printf("    mov [rax], rdi\n");
        printf("    push rdi\n");
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->ty) {
    case '+':
        printf("    add rax, rdi\n");
        break;
    case '-':
        printf("    sub rax, rdi\n");
        break;
    case '*':
        printf("    imul rdi\n");
        break;
    case '/':
        printf("    cqo\n");
        printf("    idiv rdi\n");
        break;
    case ND_EQ:
        printf("    cmp rax, rdi\n");
        printf("    sete al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_NE:
        printf("    cmp rax, rdi\n");
        printf("    setne al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LE:
        printf("    cmp rax, rdi\n");
        printf("    setle al\n");
        printf("    movzb rax, al\n");
        break;
    case '>':
        printf("    cmp rdi, rax\n");
        printf("    setl al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_GE:
        printf("    cmp rdi, rax\n");
        printf("    setle al\n");
        printf("    movzb rax, al\n");
        break;
    case '<':
        printf("    cmp rax, rdi\n");
        printf("    setl al\n");
        printf("    movzb rax, al\n");
        break;
    }

    printf("    push rax\n");
};