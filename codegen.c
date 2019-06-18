#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mikan.h"

int pos = 0;

int jump_count = 0;

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

    if (!consume(';')) {
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
            //Token *t = tokens->data[pos];
            error_at(t->input, "開き括弧に対応する閉じ括弧がありません");
        }
        return node;
    }

    if (consume(TK_NUM)) {
        //Token *t = tokens->data[pos];
        return new_node_num(t->val);
    }

    if (consume(TK_IDENT)) {
        
        if (consume('(')) {
            Node *node = malloc(sizeof(Node));
            node->ty = ND_FUNC;
            node->name = t->name;
            node->args = new_vector();

            while (!consume(')')) {
                vec_push(node->args, term());
                consume(',');
            }

            return node;
        }

        if (map_exists(identities, t->name) == 1) {
            int offset = (int)map_get(identities, t->name);
            Node *node = new_node_ident(offset);
            return node;
        }

        int offset = (count + 1) * 8;
        Node *node = new_node_ident(offset);
        map_put(identities, t->name, offset);
        count++;
        return node;
    }
    
    //Token *t = tokens->data[pos];
    error_at(t->input, "数値でも開き括弧でもないトークンです");
    exit(1);
}

void gen_lval(Node *node) {
    if (node->ty != ND_LVAR)
        error("代入の左辺値が変数ではありません");

    printf("    mov rax, rbp\n");
    printf("    sub rax, %d\n", node->offset);
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
        gen(node->cond);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        if (node->els) {
            printf("    je  .Lelse%d\n", j1);
        }
        gen(node->then);
        printf("    je  .Lend%d\n", j2);
        if (node->els) {
            printf(".Lelse%d:\n", j1);
            gen(node->els);
        }
        printf(".Lend%d:\n", j2);
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
        for (int j = 0; j < block->len; j++) {
            gen(block->data[j]);
        }
        return;
    }

    if (node->ty == ND_FUNC) {
        Vector *args = node->args;
        for (int j = args->len -1; j >= 0; j--) {
            Node *n = args->data[j];
            switch (j) {
            case 0:
                printf("    mov rdi, %d\n", n->val);
                break;
            case 1:
                printf("    mov rsi, %d\n", n->val);
                break;
            case 2:
                printf("    mov rdx, %d\n", n->val);
                break;
            case 3:
                printf("    mov rcx, %d\n", n->val);
                break;
            case 4:
                printf("    mov r8, %d\n", n->val);
                break;
            case 5:
                printf("    mov r9, %d\n", n->val);
                break;
            }
        }
        printf("    call %s\n", node->name);
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
    case '<':
        printf("    cmp rax, rdi\n");
        printf("    setl al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_GE:
        printf("    cmp rdi, rax\n");
        printf("    setle al\n");
        printf("    movzb rax, al\n");
        break;
    case '>':
        printf("    cmp rdi, rax\n");
        printf("    setl al\n");
        printf("    movzb rax, al\n");
        break;
    }

    printf("    push rax\n");
}