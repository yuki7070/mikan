#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mikan.h"

int jump_count = 0;

int valid_type(int ty) {
    if (ty == INT || ty == PTR || ty == ARRAY) {
        return 1;
    }
    return 0;
}

void func_lval(Node *parent, Node *node) {

    if (node->ty != ND_LVAR && node->ty != ND_DVAR) {
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
        Node *parent_node = map_get(parent_idents, node->name);
        offset = parent_node->offset - (parent_idents->keys->len+1)*8;
        node->type = parent_node->type;
    } else if (map_exists(idents, node->name) == 1) {
        Node *get_node = map_get(idents, node->name);
        offset = get_node->offset;
        node->type = get_node->type;
    } else {
        if (!valid_type(node->type->ty)) {
            Token *t = node->token;
            error_at(t->input, "変数の型が不明です");
        }
        int stuck_size = 0;
        for (int j = 0; j < idents->vals->len; j++) {
            Node *n = idents->vals->data[j];
            if (stuck_size < n->offset) {
                stuck_size = n->offset;
            }
        }

        if (node->type->ty == PTR) {
            Type *t = node->type;
            int size;
            if (node->type->ptr_to->ty == INT) {
                size = 8;
            } else if (node->type->ptr_to->ty == PTR) {
                size = 8;
            }
            offset = stuck_size + 8 + size;
        } else if (node->type->ty == INT) {
            offset = stuck_size + 8;
        } else if (node->type->ty == ARRAY) {
            Type *t = node->type;
            int size;
            if (node->type->ptr_to->ty == INT) {
                size = 8;
            } else if (node->type->ptr_to->ty == PTR) {
                size = 8;
            }
            
            offset = stuck_size + (int)t->array_size * size;
        }
        node->offset = offset;
        map_put(idents, node->name, node);
    }

    node->offset = offset;
    return;
}

void dec_arg(Node *node) {
    Token *t = node->token;
    if (node->ty != ND_DVAR)
        error_at(t->input, "何かがおかしい？？");

    printf("    mov rax, rbp\n");
    if (node->offset < 0) 
        error_at(t->input, "何かがおかしい？？");
        
    printf("    sub rax, %d\n", node->offset);
    printf("    push rax\n");
}

void gen_lval(Node *node) {
    if (node->ty == ND_DEREF) {
        if (node->lhs->type && node->lhs->type->ty == PTR) {
            gen(node->lhs);
            printf("    pop rax\n");
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
        } else {
            gen(node->lhs);
        }
        return;
    }

    if (node->ty != ND_LVAR && node->ty != ND_DVAR) {
        printf("%d\n", node->ty);
        printf("%d\n", '+');
        error("代入の左辺値が変数ではありません");
    }

    if (node->ty == ND_DVAR && node->type->ty == PTR) {
        printf("    mov rax, rbp\n");
        if (node->offset < 0) {
            printf("    add rax, %d\n", node->offset*(-1));
        } else {
            printf("    sub rax, %d\n", node->offset);
        }
        printf("    push rax\n");
        printf("    mov rax, rbp\n");
        if (node->offset < 0) {
            printf("    sub rax, %d\n", (node->offset+8)*(-1));
        } else {
            printf("    sub rax, %d\n", (node->offset-8));
        }
        printf("    push rax\n");
        printf("    pop rdi\n");
        printf("    pop rax\n");
        printf("    mov [rax], rdi\n");
    }

    /*
    if (node->ty == ND_LVAR && node->type->ty == ARRAY) {
        printf("ARE\n");
        
        gen(node->index);
        int size = 0;
        if (node->type->ptr_to == PTR) {
            size = 8;
        } else if (node->type->ptr_to == INT) {
            size = 8;
        }
        printf("    pop rax\n");
        printf("    imul rax, %d\n", size);
        if (node->offset < 0) {
            printf("    mov rdi, %d\n", node->offset*(-1));
        } else {
            printf("    mov rdi, %d\n", node->offset);
        }
        printf("    sub rdi, rax\n");

        printf("    mov rax, rbp\n");
        if (node->offset < 0) {
            printf("    add rax, rdi\n");
        } else {
            printf("    sub rax, rdi\n");
        }
        printf("    push rax\n");
        
        return;
    }
    */

   if (node->ty == ND_LVAR && node->type->ty == ARRAY) {
       //printf("TESTARRAY\n");
   }

    printf("    mov rax, rbp\n");
    if (node->offset < 0) {
        printf("    add rax, %d\n", node->offset*(-1));
    } else {
        printf("    sub rax, %d\n", node->offset);
    }
    printf("    push rax\n");
}

void calc_ptr(Node *node) {
    gen(node->lhs);
    gen(node->rhs);
    //printf("calcの中の中\n");

    int lhs = is_ptr_or_array(node->lhs);
    //printf("TESTcalc\n");
    int rhs = is_ptr_or_array(node->rhs);
    //printf("TESTcalc2\n");

    int lhs_ptr = is_ptr(node->lhs);
    //printf("TESTcalc3\n");
    //printf("%d\n", node->rhs);
    int rhs_ptr = is_ptr(node->rhs);
    //printf("TESTcalc4\n");

    if (lhs == 0 && rhs == 0)
        return;
    
    if (lhs != 0 && rhs != 0)
        return;

    if (lhs_ptr || rhs_ptr) {
        //printf("TESTTEST\n");
        //printf("rhs_%d\n", rhs);
        //printf("lhs_%d\n", lhs);
        if (rhs == 0) {
            printf("    pop rdi\n");
            printf("    pop rax\n");
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
            printf("    push rdi\n");
        }

        if (lhs == 0) {
            printf("    pop rax\n");
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
        }
    }
    
    printf("    pop rax\n");
    if (rhs == 0) {
        if (lhs == INT) {
            printf("    imul rax, 8\n");
        } else if (lhs == PTR) {
            printf("    imul rax, 8\n");
        }
    }

    if (lhs == 0) {
        printf("    pop rdi\n");
        if (rhs == INT) {
            printf("    imul rdi, 8\n");
        } else if (rhs == PTR) {
            printf("    imul rdi, 8\n");
        }
        printf("    push rdi\n");
    }

    printf("    push rax\n");

}

int is_ptr_or_array(Node *node) {
    if (node->type != NULL && (node->type->ty == PTR || node->type->ty == ARRAY)) {
        return ptr_type(node->type->ptr_to);
    }
    if (node->ty == ND_DEREF) {
        //printf("TEST\n");
        Node *child = node->lhs;
        if (child->type == NULL) {
            //printf("TEST+++++\n");
            int lhs = is_ptr_or_array(child->lhs);
            int rhs = is_ptr_or_array(child->rhs);
            //printf("%d\n", lhs);
            //printf("%d\n", rhs);
            if (lhs == 0 && rhs == 0) {
                error("ポインタじゃないよ!");
                return 0;
            }
            if (lhs <= 1  && rhs <= 1)
                return 0;
            if (lhs != 0) {
                return lhs;
            }
            if (rhs != 0)
                return rhs;
        }
        if (child->type->ptr_to->ty == PTR || child->type->ptr_to->ty == ARRAY)
            return ptr_type(child);
        return 0;
    }
    if (node->lhs)
        return is_ptr_or_array(node->lhs);
    if (node->rhs)
        return is_ptr_or_array(node->rhs);
    return 0;
}

int is_ptr(Node *node) {
    //printf("TYPE %d\n", node->ty);
    if (node->type != NULL && node->type->ty == PTR) {
        //printf("TEST_is_ptr\n");
        return ptr_type(node->type->ptr_to);
    }
    if (node->ty == ND_DEREF) {
        Node *child = node->lhs;
        //printf("ここか！？！？_%d\n", child->ty);
        //printf("%d\n", '+');
        if (!child->type) {
            //printf("are\n");
            int left = is_ptr(child->lhs);
            int right = is_ptr(child->rhs);
            if (left != 0) {
                return ptr_type(child->lhs);
            }
            if (right != 0) {
                return ptr_type(child->rhs);
            }
            return 0;
            
        }
        if (child->type->ptr_to->ty == PTR) {
            //printf("じゃあ、ここか！？！？\n");
            return ptr_type(child);
        }
        //printf("じゃあ、ここか2！？！？\n");
        return 0;
    }
    if (node->lhs)
        return is_ptr(node->lhs);
    if (node->rhs)
        return is_ptr(node->rhs);
    return 0;
}

int ptr_type(Type *type) {
    if (type->ty != PTR && type->ty != ARRAY) {
        return type->ty;
    }
    return ptr_type(type->ptr_to);
}

void check_type(Node *node) {
    if (node->ty == ND_NUM || node->type->ty == INT) {
        printf("    push 4\n");
        return;
    }
    if (node->type->ty == PTR) {
        printf("    push 8\n");
        return;
    }
    if (node->lhs)
        return check_type(node->lhs);
    if (node->rhs)
        return check_type(node->rhs);

    Token *t = node->token;
    error_at(t->input, "何かがおかしいかも...?");
}

void gen(Node *node) {
    if (node->ty == ND_RETURN) {
        //printf("TEST_RETURN\n");
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
        
        for (int j = 0; j < block->len; j++) {
            Node *n = block->data[j];
            gen(n);
            if (n->ty != ND_RETURN) {
                printf("    pop rax\n");
            }
        }

        return;
    }

    if (node->ty == ND_FUNC) {
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

    if (node->ty == ND_DFUNC) {
        Vector *args = node->args;
        Vector *block = node->block;
        Map *idents = new_map();
        node->idents = idents;

        int stuck_size = 0;

        for (int j = 0; j < args->len; j++) {
            func_lval(node, args->data[j]);
        }
        for (int j = 0; j < block->len; j++) {
            func_lval(node, block->data[j]);
        }

        for (int j = 0; j < idents->vals->len; j++) {
            Node *n = idents->vals->data[j];
            if (stuck_size < n->offset) {
                stuck_size = n->offset;
            }
        }

        printf("%s: \n", node->name);
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, %d\n", stuck_size);

        for (int j = 0; j < args->len; j++) {
            Node *n = args->data[j];
            dec_arg(n);
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

    if (node->ty == ND_DEREF) {
        //printf("DEREF_START\n");
        gen(node->lhs);
        //printf("TEST\n");
        printf("    pop rax\n");
        printf("    mov rax, [rax]\n");
        printf("    push rax\n");

        //int lhs_ptr = is_ptr(node->lhs);
        //int lhs_ptr_array = is_ptr_or_array(node->lhs);
        //printf("%d\n", lhs_ptr);
        //printf("%d\n", lhs_ptr_array);

        if (is_ptr(node->lhs) != 0 && node->lhs->ty == ND_LVAR) {
            //printf("TEST\n");
            printf("    pop rax\n");
            printf("    mov rax, [rax]\n");
            printf("    push rax\n");
        }
        //printf("DEREFE_END\n");
        return;
    }

    if (node->ty == ND_LVAR) {
        gen_lval(node);
        printf("    pop rax\n");
        
        if (node->type->ty != PTR && node->type->ty != ARRAY) {
            printf("    mov rax, [rax]\n");
        }
        
        printf("    push rax\n");
        return;
    }

    if (node->ty == ND_DVAR) {
        gen_lval(node);
        
        return;
    }

    if (node->ty == '=') {
        //printf("TEST\n");
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("    pop rdi\n");
        printf("    pop rax\n");
        printf("    mov [rax], rdi\n");
        printf("    push rdi\n");
        //printf("TEST2\n");
        return;
    }

    if (node->ty == ND_ADDR) {
        gen_lval(node->lhs);
        return;
    }

    if (node->ty == '+') {
        //printf("TEST_+\n");
        calc_ptr(node);
        //printf("TEST_+_2\n");

        printf("    pop rdi\n");
        printf("    pop rax\n");

        printf("    add rax, rdi\n");
        printf("    push rax\n");
        //printf("+のOQARI\n");
        return;
    }

    if (node->ty == ND_SIZEOF) {
        check_type(node->lhs);
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