#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TK_PUNCT,   // Punctuators
    TK_NUM,
    TK_EOF,
} TokenType;

typedef struct Token Token;
struct Token {
    TokenType type;
    Token *next;
    int val;
    char *loc;
    int len;
};

// Input string
char *current_input;

// Report an error location and exit.
static void verror_at(char *loc, char *fmt, va_list ap) {
    int pos = loc - current_input;
    fprintf(stderr, "%s\n", current_input);
    fprintf(stderr, "%*s", pos, ""); // print pos spaces
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

static void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

static void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
}

static void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}



static bool equal(Token *tok, char *s) {
    return memcmp(tok->loc, s, tok->len) == 0 && s[tok->len] == '\0';
}

static Token *skip(Token *tok, char *s) {
    if (!equal(tok, s)) {
        error_tok(tok, "expected '%s'", s);
    }
    return tok->next;
}

static int get_number(Token *tok) {
    if (tok->type != TK_NUM) {
        error_tok(tok, "expected a number");
    }
    return tok->val;
}

static Token *new_token(TokenType type, char *start, char *end) {
    Token *tok = calloc(1, sizeof(Token));
    tok->type = type;
    tok->loc = start;
    tok->len = end - start;
    return tok;
}

static Token *tokenize(void) {
    char *p = current_input;
    Token head = {};
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            ++p;
            continue;
        }

        if (isdigit(*p)) {
            cur = cur->next = new_token(TK_NUM, p, p);
            char *q = p;
            cur->val = strtoul(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        if(*p == '-' || *p == '+') {
            cur = cur->next = new_token(TK_PUNCT, p, p + 1);
            ++p;
            continue;
        }

        error_at(p, "invalid token");
    }
    cur = cur->next = new_token(TK_EOF, p, p);
    return head.next;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    current_input = argv[1];
    Token *tok = tokenize();

    printf("  .globl main\n");
    printf("main:\n");

    printf("  mov $%ld, %%rax\n", get_number(tok));
    tok = tok->next;

    while(tok->type != TK_EOF) {
        if (equal(tok, "+")) {
            printf("  add $%ld, %%rax\n", get_number(tok->next));
            tok = tok->next->next;
            continue;
        }

        tok = skip(tok, "-");
        printf("  sub $%ld, %%rax\n", get_number(tok));
        tok = tok->next;

    }

    printf("  ret\n");
    return 0;
}