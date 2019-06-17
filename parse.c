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

        if (strchr("+-*/()<>=;", *p)) {
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