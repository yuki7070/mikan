#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mikan.h"

int jump_seq = 0;
int loop_seq = 0;
int break_pnt = 0;
int continue_pnt = 0;

Node *var_info(Node *parent, Node *node);

void gen_str_literals(Vector *vec) {
    for (int j = 0; j < vec->len; j++) {
        Node *node = vec->data[j];
        printf(".LC%d:\n", node->offset);
        printf("    .string \"%s\"\n", node->name);
    }
}

void gen_lval(Node *node) {
    if (node->ty == ND_DEREF) {
        return gen(node->lhs);;
    }
    if (node->ty == ND_ADDR) {

    }
    if (node->ty != ND_LVAR && node->ty != ND_DVAR) {
        return gen(node);
    }

    Node *info = var_info(node->parent, node);
    if (!info) {
        printf("ERROR no var name %s\n", node->name);
        printf("%d\n", node->parent->ty);
        exit(1);
    }

    if (info->ty != ND_GVAR) {
        int offset = var_offset(node->parent, node);
        printf("    mov rax, rbp\n");
        printf("    sub rax, %d\n", offset);
        printf("    push rax\n");
    } else {
        printf("    lea rax, %s\n", info->name);
        printf("    push rax\n");
    }

    return;
}

int type_size(int ty) {
    switch (ty) {
    case TY_INT:
        return 4;
    case TY_CHAR:
        return 1;
    case TY_PTR:
        return 8;
    case TY_VOID:
        return 0;
    }
    return 0;
}

int var_offset(Node *parent, Node *node) {
    if (!parent->idents)
        return 0;

    Node *var_info = malloc(sizeof(Node));

    if (map_exists(parent->idents, node->name) == 1) {
        var_info = map_get(parent->idents, node->name);
        return var_info->offset;
    } else if (parent->parent) {
        return var_offset(parent->parent, node);
    }

    return 0;
}

Node *var_info(Node *parent, Node *node) {
    if (!parent)
        return NULL;
    if (!parent->idents)
        return NULL;

    Node *info = malloc(sizeof(Node));

    if (map_exists(parent->idents, node->name) == 1) {
        info = map_get(parent->idents, node->name);
        return info;
    } else if (parent->parent) {
        return var_info(parent->parent, node);
    }

    return NULL;
}

int node_type(Node *node) {
    Node *n = malloc(sizeof(Node));
    if (node->ty == ND_DEREF) {
        n = var_info(node->lhs->parent, node->lhs);
        if (n != NULL)
            return type_size(n->type->ptr_to->ty);

        n = var_info(node->lhs->lhs->parent, node->lhs->lhs);
        if (n != NULL)
            return type_size(n->type->ptr_to->ty);

        n = var_info(node->lhs->rhs->parent, node->lhs->rhs);
        if (n != NULL)
            return type_size(n->type->ptr_to->ty);
    }

    if (node->ty == ND_ADDR) {
        return type_size(TY_PTR);
    }

    if (node->ty == ND_LVAR) {
        n = var_info(node->parent, node);
        return type_size(n->type->ty);
    }
}

void calc_ptr_stuck(int left_ty, int right_ty,  Node *node) {
    if (left_ty) {
        gen_lval(node);
        Node *info = var_info(node->parent, node);
        if (info->type->ty == TY_PTR) {
            printf("    pop rax\n");
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
        }
    } else {
        gen(node);
        if (right_ty) {
            printf("    push %d\n", type_size(right_ty));
            printf("    pop rdi\n");
            printf("    pop rax\n");
            printf("    imul rax, rdi\n");
            printf("    push rax\n");
        }
    }
}

void gen_return(Node *node) {
    if (node->lhs) {
        gen(node->lhs);
        printf("    pop rax\n");
    }
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
    return;
}

void gen_if(Node *node) {
    int seq = jump_seq++;

    gen(node->cond);

    Vector *then = node->then;
    Vector *els = node->els;

    printf("    pop rax\n");
    printf("    cmp rax, 0\n");
    
    if (node->els) {
        printf("    je  .LLelse%d\n", seq);
        for (int j = 0; j < then->len; j++) {
            gen(then->data[j]);
            printf("    pop rax\n");
        }
        printf("    jmp  .LLend%d\n", seq);
        printf(".LLelse%d:\n", seq);
        for (int j = 0; j < els->len; j++) {
            gen(els->data[j]);
            printf("    pop rax\n");
        }
    } else {
        printf("    je  .LLend%d\n", seq);
        for (int j = 0; j < then->len; j++) {
            gen(then->data[j]);
            printf("    pop rax\n");
        }
    }
    printf(".LLend%d:\n", seq);
    printf("    push rax\n");
    
    return;
}

void gen_for(Node *node) {
    int seq = loop_seq++;
    int tmp_break = break_pnt;
    int tmp_cotinue = continue_pnt;
    break_pnt = seq;
    continue_pnt = seq;

    Vector *loop = node->loop;

    if (node->init) {
        gen(node->init);
    }
    printf(".Lbegin%d:\n", seq);
    if (node->cond) {
        gen(node->cond);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lend%d\n", seq);
    }

    for (int j = 0; j < loop->len; j++) {
        gen(loop->data[j]);
        printf("    pop rax\n");
    }

    printf(".Lcontinue%d:", continue_pnt);
    if (node->inc) {
        gen(node->inc);
        printf("    pop rax\n");
    }

    printf("    jmp .Lbegin%d\n", seq);
    printf(".Lend%d:\n", seq);
    printf(".Lbreak%d:\n", seq);
    printf("    push rax\n");
    break_pnt = tmp_break;
    continue_pnt = tmp_cotinue;
    return;
}

void gen_while(Node *node) {
    int seq = loop_seq++;
    int tmp_break = break_pnt;
    int tmp_cotinue = continue_pnt;
    break_pnt = seq;
    continue_pnt = seq;

    Vector *loop = node->loop;

    printf(".Lbegin%d:\n", seq);
    printf(".Lcontinue%d:\n", seq);
    if (node->cond) {
        gen(node->cond);
        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lend%d\n", seq);
    }

    for (int j = 0; j < loop->len; j++) {
        gen(loop->data[j]);
        printf("    pop rax\n");
    }

    printf("    jmp .Lbegin%d\n", seq);
    printf(".Lend%d:\n", seq);
    printf(".Lbreak%d:\n", seq);
    printf("    push rax\n");
    break_pnt = tmp_break;
    continue_pnt = tmp_cotinue;
    return;
}

void gen_call_func(Node *node) {
    printf("    mov al, 0\n");
    Vector *args = node->args;
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

void gen_decl_func(Node *node) {
    Vector *args = node->args;
    Vector *block = node->block;

    printf("%s: \n", node->name);
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", node->offset);
        
    for (int j = 0; j < args->len; j++) {
        Node *n = args->data[j];
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
        printf("    pop rax\n");
    }
}

void gen_local_var(Node *node) {
    gen_lval(node);

    int size = node_type(node);
    
    printf("    pop rax\n");
    
    if (size == 8) {
        printf("    mov rax, [rax]\n");
    } else if (size == 4) {
        printf("    mov eax, [rax]\n");
    } else if (size == 1) {
        printf("    mov al, [rax]\n");
    }

    printf("    push rax\n");
    return;
}

void gen_global_var(Node *node) {
    printf(".bss\n");
    printf("%s:\n", node->name);
    printf("    .zero %d\n", node->offset);
    printf(".text\n");
    return;
}

void gen_decl_local_var(Node *node) {
    return;
}

void gen_assign(Node *node) {
    gen_lval(node->lhs);
    if (node->rhs->ty == ND_LVAR) {
        Node *info = var_info(node->rhs->parent, node->rhs);
        if (info->type->ty == TY_ARRAY) {
            gen_lval(node->rhs);
        } else {
            gen(node->rhs);
        }
    } else {
        gen(node->rhs);
    }
    
    int size = node_type(node->lhs);
    
    printf("    pop rdi\n");
    printf("    pop rax\n");
    
    if (size == 8) {
        printf("    mov [rax], rdi\n");
    } else if (size == 4) {
        printf("    mov [rax], edi\n");
    } else if (size == 1) {
        printf("    mov [rax], dil\n");
    }
    
    printf("    push rdi\n");
    return;
}

void gen_indirection(Node *node) {
    gen(node->lhs);

    /*
    if (node->lhs->ty == ND_LVAR) {
        Node *info = var_info(node->lhs->parent, node->lhs);
        if (info->type->ty == TY_ARRAY)
            return;
    }*/

    printf("    pop rax\n");
    printf("    mov rax, [rax]\n");
    printf("    push rax\n");
    
    return;
}

void gen_address(Node *node) {
    gen_lval(node->lhs);
    return;
}

void gen_sizeof(Node *node) {
    Node *info = var_info(node->parent, node->lhs);
    int size = type_size(info->type->ty);
    printf("    push %d\n", size);
    return;
}

void gen_add(Node *node) {
    int is_left_ptr = 0;
    int is_right_ptr = 0;
    if (node->lhs->ty == ND_LVAR) {
        Node *info = var_info(node->lhs->parent, node->lhs);
        if (info->type->ptr_to) {
            is_left_ptr = info->type->ptr_to->ty;
        }
    }
    if (node->rhs->ty == ND_LVAR) {
        Node *info = var_info(node->rhs->parent, node->rhs);
        if (info->type->ptr_to) {
            is_right_ptr = info->type->ptr_to->ty;
        }
    }

    calc_ptr_stuck(is_left_ptr, is_right_ptr, node->lhs);
    calc_ptr_stuck(is_right_ptr, is_left_ptr, node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");
    printf("    add rax, rdi\n");
    printf("    push rax\n");
    return;
}

void gen_string(Node *node) {
    printf("    lea rax, .LC%d\n", node->offset);
    printf("    push rax\n");
    return;
}

void gen_logic_and(Node *node) {
    int seq = jump_seq++;
    gen(node->lhs);
    printf("    pop rax\n");
    printf("    cmp rax, 0\n");
    printf("    je .LLfalse%d\n", seq);
    gen(node->rhs);
    printf("    pop rax\n");
    printf("    cmp rax, 0\n");
    printf("    je .LLfalse%d\n", seq);
    printf("    push 1\n");
    printf("    jmp .LLend%d\n", seq);
    printf(".LLfalse%d:\n", seq);
    printf("    push 0\n");
    printf(".LLend%d:\n", seq);
    return;
}

void gen_logic_or(Node *node) {
    int seq = jump_seq++;
    gen(node->lhs);
    printf("    pop rax\n");
    printf("    cmp rax, 0\n");
    printf("    jne .LLtrue%d\n", seq);
    gen(node->rhs);
    printf("    pop rax\n");
    printf("    cmp rax, 0\n");
    printf("    jne .LLtrue%d\n", seq);
    printf("    push 0\n");
    printf("    jmp .LLend%d\n", seq);
    printf(".LLtrue%d:\n", seq);
    printf("    push 1\n");
    printf(".LLend%d:\n", seq);
    return;
}

void gen_break(Node *node) {
    printf("    jmp .Lend%d\n", break_pnt);
    return;
}

void gen_continue(Node *node) {
    printf("    jmp .Lcontinue%d\n", continue_pnt);
    return;
}

void gen(Node *node) {
    if (node->ty == ND_RETURN) {
        return gen_return(node);
    }

    if (node->ty == ND_IF) {
        return gen_if(node);
    }

    if (node->ty == ND_FUNC) {
        return gen_call_func(node);
    }

    if (node->ty == ND_DFUNC) {
        return gen_decl_func(node);
    }

    if (node->ty == ND_LVAR) {
        return gen_local_var(node);
    }

    if (node->ty == ND_GVAR) {
        return gen_global_var(node);
    }

    if (node->ty == ND_DVAR) {
        return gen_decl_local_var(node);
    }

    if (node->ty == '=') {
        return gen_assign(node);
    }

    if (node->ty == '+') {
        return gen_add(node);
    }

    if (node->ty == ND_DEREF) {
        return gen_indirection(node);
    }

    if (node->ty == ND_ADDR) {
        return gen_address(node);
    }

    if (node->ty == ND_SIZEOF) {
        return gen_sizeof(node);
    }

    if (node->ty == ND_STR) {
        return gen_string(node);
    }

    if (node->ty == ND_FOR) {
        return gen_for(node);
    }

    if (node->ty == ND_WHILE) {
        return gen_while(node);
    }

    if (node->ty == ND_AND) {
        return gen_logic_and(node);
    }

    if (node->ty == ND_OR) {
        return gen_logic_or(node);
    }

    if (node->ty == ND_BREAK) {
        return gen_break(node);
    }

    if (node->ty == ND_CONTINUE) {
        return gen_continue(node);
    }

    if (node->ty == ND_BLOCK) {
        Vector *block = node->block;
        
        Map *idents = new_map();
        node->idents = idents;
        
        for (int j = 0; j < block->len; j++) {
            Node *n = block->data[j];
            gen(n);
            if (n->ty != ND_RETURN) {
                printf("    pop rax\n");
            }
        }

        return;
    }

    if (node->ty == ND_NUM) {
        printf("    push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->ty) {
    case '-':
        printf("    sub rax, rdi\n");
        break;
    case '*':
        printf("    imul rax, rdi\n");
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