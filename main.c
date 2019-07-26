#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "mikan.h"

char *read_file(char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp)
        error("can not open %s: %s", path, strerror(errno));

    if (fseek(fp, 0, SEEK_END) == -1)
        error("%s: fseek: %s", path, strerror(errno));

    size_t size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) == -1)
        error("%s: fseek: $s", path, strerror(errno));

    char *buf = calloc(1, size + 2);
    fread(buf, size, 1, fp);

    if (size == 0 || buf[size - 1] != '\n')
        buf[size++] = '\n';

    buf[size] = '\0';
    fclose(fp);
    return buf;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
        return 1;
    }

    filepath = argv[1];

    if (strcmp(filepath, "-test") == 0) {
        runtest();
        return 0;
    }

    int len = strlen(filepath);
    char *suffix = filepath+(len-2);
    if (strcmp(suffix, ".c") == 0) {
        is_file = 1;
        user_input = read_file(filepath);
    } else {
        user_input = filepath;
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