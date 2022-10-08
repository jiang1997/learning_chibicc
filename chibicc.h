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

typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NEG,
    ND_EQ,          // ==
    ND_NE,          // !=
    ND_LT,          // <
    ND_LE,          // <=
    ND_EXPR_STMT,   // Expression statement
    ND_NUM,
} NodeType;

// AST node type
typedef struct Node Node;
struct Node {
    NodeType type;
    Node *next;
    Node *lhs;
    Node *rhs;
    int val;
};

void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
void error(char *fmt, ...);
Token *skip(Token *tok, char *s);
bool equal(Token *tok, char *s);

Token *tokenize(char *input);
Node *parse(Token *tok);
void codegen(Node *node);