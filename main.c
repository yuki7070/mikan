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
    char_literals = new_vector();

    tokens = tokenize();
    program();

    printf(".intel_syntax noprefix\n");
    gen_str_literals(char_literals);
    printf(".global main\n");

    for (int i = 0; code[i]; i++) {
        Node *n = code[i];
        gen(code[i]);
    }

    return 0;
}