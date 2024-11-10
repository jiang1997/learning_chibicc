#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node Node;

// local variable
typedef struct Obj Obj;
struct Obj {
    Obj *next;
    char *name; // Variable name
    int offset; // Offset from rbp
    int length;
};

// Function
typedef struct Function Function;
struct Function {
    Node *body;
    Obj *locals;
    int stack_size;
};

typedef enum {
    TK_PUNCT, // Punctuators
    TK_NUM,
    TK_EOF,
    TK_IDENT, // Identifiers
    TK_KEYWORD, // Keywords
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
    ND_EQ, // ==
    ND_NE, // !=
    ND_LT, // <
    ND_LE, // <=
    ND_ASSIGN,
    ND_RETURN_STMT, // return statement
    ND_EXPR_STMT, // Expression statement
    ND_NUM,
    ND_VAR,
    ND_BLOCK,
    ND_IF,
} NodeType;

// AST node type

struct Node {
    NodeType type;
    Node *next;
    Node *lhs;
    Node *rhs;
    Node *body;
    char name;
    int val;
    Obj *var;
};

void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
void error(char *fmt, ...);
Token *skip(Token *tok, char *s);
bool equal(Token *tok, char *s);

Token *tokenize(char *input);
Function *parse(Token *tok);
void codegen(Function *prog);
