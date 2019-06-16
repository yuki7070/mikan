#include <ctype.h>
#include <stdlib.h>
#include "mikan.h"

Token *add_token(Vector *vec, int ty, char *input) {
    Token *t = malloc(sizeof(Token));
    t->ty = ty;
    t->input = input;
    vec_push(vec, t);
    return t;
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

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '<' || *p == '>') {
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

        error_at(p, "トークナイズできません");
    }
    
    add_token(tokens, TK_EOF, p);

    return tokens;
}