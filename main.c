#include <string.h>
#include <stdio.h>
#include "mikan.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
        return 1;
    }

    user_input = argv[1];

    if (strcmp(user_input, "-test") == 0) {
        runtest();
        return 0;
    }

    count = 0;

    identities = new_map();
    functions = new_map();

    tokens = tokenize();

    program();

    //printf("==========codegen=========\n");

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    //printf("main: \n");

    /*
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, %d\n", count);
    */

    for (int i = 0; code[i]; i++) {
        //printf("TEST\n");
        Node *n = code[i];
        //printf("TEST2\n");
        //printf("TEST%d\n", n->ty);
        gen(code[i]);

        //printf("    pop rax\n");
    }

    /*
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
    */

    return 0;
}