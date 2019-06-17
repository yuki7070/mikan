#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "mikan.h"

Token *add_token(Vector *vec, int ty, char *input) {
    Token *t = malloc(sizeof(Token));
    t->ty = ty;
    t->input = input;
    vec_push(vec, t);
    return t;
}

int is_alnum(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || (c == '_');
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

        if (*p == '=' && *(p+1) == '=') {
            //演算子==の実装
            add_token(tokens, TK_EQ, p);
            i++;
            p += 2;
            continue;
        }

        if (*p == '!' && *(p+1) == '=') {
            //演算子!=の実装
            add_token(tokens, TK_NE, p);
            i++;
            p += 2;
            continue;
        }

        if (*p == '<' && *(p+1) == '=') {
            //演算子<=の実装
            add_token(tokens, TK_LE, p);
            i++;
            p += 2;
            continue;
        }

        if (*p == '>' && *(p+1) == '=') {
            //演算子>=の実装
            add_token(tokens, TK_GE, p);
            i++;
            p += 2;
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '<' || *p == '>' || *p == '=' || *p == ';') {
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

        if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
            add_token(tokens, TK_RETURN, p);
            i++;
            p += 6;
            continue;
        }

        if ('a' <= *p && *p <= 'z') {
            add_token(tokens, TK_IDENT, p);
            i++;
            p++;
            continue;
        }

        error_at(p, "トークナイズできません");
    }
    
    add_token(tokens, TK_EOF, p);

    return tokens;
}