#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mikan.h"

Token *add_token(Vector *vec, int ty, char *input) {
    Token *t = malloc(sizeof(Token));
    t->ty = ty;
    t->input = input;
    vec_push(vec, t);
    return t;
}

char *strndup(const char *s, size_t n) {
    char *p = memchr(s, '\0', n);
    if (p != NULL)
        n = p - s;
    p = malloc(n + 1);
    if (p != NULL) {
        memcpy(p, s, n);
        p[n] = '\0';
    }
    return p;
}

int is_alnum(char c) {
    return isalpha(c) || isdigit(c) || (c == '_');
}

Vector *tokenize() {
    Vector *tokens = new_vector();
    char *p = user_input;

    int i = 0;
    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (strncmp(p, "//", 2) == 0) {
            p += 2;
            while (*p != '\n')
                p++;
            continue;
        }

        if (strncmp(p, "/*", 2) == 0) {
            char *q = strstr(p + 2, "*/");
            if (!q)
                error_at(p, "コメントが閉じられていません");
            p = q + 2;
            continue;
        }

        if (strncmp(p, "==", 2) == 0) {
            //演算子==の実装
            add_token(tokens, TK_EQ, p);
            i++;
            p += 2;
            continue;
        }

        if (strncmp(p, "!=", 2) == 0) {
            //演算子!=の実装
            add_token(tokens, TK_NE, p);
            i++;
            p += 2;
            continue;
        }

        if (strncmp(p, "<=", 2) == 0) {
            //演算子<=の実装
            add_token(tokens, TK_LE, p);
            i++;
            p += 2;
            continue;
        }

        if (strncmp(p, ">=", 2) == 0) {
            //演算子>=の実装
            add_token(tokens, TK_GE, p);
            i++;
            p += 2;
            continue;
        }

        if (strncmp(p, "&&", 2) == 0) {
            add_token(tokens, TK_AND, p);
            i++;
            p += 2;
            continue;
        }

        if (strncmp(p, "||", 2) == 0) {
            add_token(tokens, TK_OR, p);
            i++;
            p += 2;
            continue;
        }

        if (*p == '"') {
            int j = 1;
            while (p[j] != '"') {
                j++;
            }
            Token *t = add_token(tokens, TK_STR, p);
            char *name = strndup(p+1, j-1);
            t->name = name;
            i++;
            p += j+1;
            continue;
        }

        if (strchr("+-*/()<>=;{},&[]", *p)) {
            add_token(tokens, *p, p);
            i++;
            p++;
            continue;
        }

        if (isdigit(*p)) {
            Token *t = add_token(tokens, TK_NUM, p);
            t->val = strtol(p, &p, 10);
            i++;
            continue;
        }

        if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
            add_token(tokens, TK_IF, p);
            i++;
            p += 2;
            continue;
        }

        if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4])) {
            add_token(tokens, TK_ELSE, p);
            i++;
            p += 4;
            continue;
        }

        if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5])) {
            add_token(tokens, TK_WHILE, p);
            i++;
            p += 5;
            continue;
        }

        if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3])) {
            add_token(tokens, TK_FOR, p);
            i++;
            p += 3;
            continue;
        }

        if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
            add_token(tokens, TK_RETURN, p);
            i++;
            p += 6;
            continue;
        }

        if (strncmp(p, "int", 3) == 0 && !is_alnum(p[3])) {
            add_token(tokens, TK_TYPE, p);
            add_token(tokens, TK_INT, p);
            i++;
            p += 3;
            continue;
        }

        if (strncmp(p, "char", 4) == 0 && !is_alnum(p[4])) {
            add_token(tokens, TK_TYPE, p);
            add_token(tokens, TK_CHAR, p);
            i++;
            p += 4;
            continue;
        }

        if (strncmp(p, "void", 4) == 0 && !is_alnum(p[4])) {
            add_token(tokens, TK_TYPE, p);
            add_token(tokens, TK_VOID, p);
            i++;
            p += 4;
            continue;
        }

        if (strncmp(p, "sizeof", 6) == 0 && !is_alnum(p[6])) {
            add_token(tokens, TK_SIZEOF, p);
            i++;
            p += 6;
            continue;
        }

        if (strncmp(p, "break", 5) == 0 && !is_alnum(p[5])) {
            add_token(tokens, TK_BREAK, p);
            i++;
            p += 5;
            continue;
        }

        if (isalpha(*p) || *p == '_') {
            int j = 1;
            while (is_alnum(p[j]))
                j++;
            
            Token *t = add_token(tokens, TK_IDENT, p);
            char *name = strndup(p, j);
            t->name = name;
            i++;
            p += j;
            continue;
        }

        error_at(p, "トークナイズできません");
    }
    
    add_token(tokens, TK_EOF, p);

    return tokens;
}