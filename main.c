#include <assert.h>
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

static bool startwith(char *p, char *q) {
    return strncmp(p, q, strlen(q)) == 0;
}

static int read_punct(char *p) {
    if (startwith(p, "==") || startwith(p, "!=") ||
        startwith(p, "<=") || startwith(p, ">="))
        return 2;
    
    return ispunct(*p) ? 1: 0;
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

        // Punctuators
        int punct_len = read_punct(p);
        if (punct_len) {
            cur = cur->next = new_token(TK_PUNCT, p, p + punct_len);
            p += cur->len;
            continue;
        }

        error_at(p, "invalid token");
    }
    cur = cur->next = new_token(TK_EOF, p, p);
    return head.next;
}


//
// Parser
//

typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NEG,
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
    ND_NUM,
} NodeType;

// AST node type
typedef struct Node Node;
struct Node {
    NodeType type;
    Node *lhs;
    Node *rhs;
    int val;
};

static Node *new_node(NodeType type) {
    Node *node = calloc(1, sizeof(Node));
    node->type = type;
    return node;
}

static Node *new_binary(NodeType type, Node *lhs, Node *rhs) {
    Node *node = new_node(type);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_num(int val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
}

static Node *new_unary(Node *lhs) {
    Node *node = new_node(ND_NEG);
    node->lhs = lhs;
}

/*

expr    = mul ("+" mul | "-" mul)*
mul     = unary ("*" unary | "/" unary)*
unary   = ("-" | "+")* unary | primary
primary = num | "(" expr ")"


expr       = equality
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? primary
primary    = num | "(" expr ")"
*/

static Node *expr(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

static Node *expr(Token **rest, Token *tok) {
    return equality(rest, tok);
}

static Node *equality(Token **rest, Token *tok) {
    Node *node = relational(&tok, tok);

    for (;;) {
        if (equal(tok, "==")) {
            node = new_binary(ND_EQ, node, relational(&tok, tok->next));
            continue;
        }

        if (equal(tok, "!=")) {
            node = new_binary(ND_NE, relational(&tok, tok->next), node);
            continue;
        }

        *rest = tok;
        return node;
    }
}

static Node *relational(Token **rest, Token *tok) {
    Node *node = add(&tok, tok);
    for (;;) {
        if (equal(tok, "<")) {
            node = new_binary(ND_LT, node, add(&tok, tok->next));
            continue;
        }

        if (equal(tok, ">")) {
            node = new_binary(ND_LT, add(&tok, tok->next), node);
            continue;
        }

        if (equal(tok, "<=")) {
            node = new_binary(ND_LE, node, add(&tok, tok->next));
            continue;
        }

        if (equal(tok, ">=")) {
            node = new_binary(ND_LE, add(&tok, tok->next), node);
            continue;
        }
        *rest = tok;
        return node;
    }

}

static Node *add(Token **rest, Token *tok) {
    Node *node = mul(&tok, tok);
    for (;;) {
        if (equal(tok, "+")) {
            node = new_binary(ND_ADD, node, mul(&tok, tok->next));
            continue;
        }

        if (equal(tok, "-")) {
            node = new_binary(ND_SUB, node, mul(&tok, tok->next));
            continue;
        }

        *rest = tok;
        return node;
    }
}

static Node *mul(Token **rest, Token *tok) {
    Node *node = unary(&tok, tok);
    for (;;) {
        if (equal(tok, "*")) {
            node = new_binary(ND_MUL, node, unary(&tok, tok->next));
            continue;
        }

        if (equal(tok, "/")) {
            node = new_binary(ND_DIV, node, unary(&tok, tok->next));
            continue;
        }

        *rest = tok;
        return node;
    }
}


static Node *unary(Token **rest, Token *tok) {
    if (equal(tok, "+")) {
        return unary(rest, tok->next);
    }

    if (equal(tok, "-")) {
        return new_unary(unary(rest, tok->next));
    }

    return primary(rest, tok);
}


// static Node *unary(Token **rest, Token *tok) {
//     Node *head = new_unary(NULL);
//     Node *node = head;
//     for (;;) {
//         if (equal(tok, "-")) {
//             node = node->lhs = new_unary(NULL);
//             tok = tok->next;
//             continue;
//         }

//         if (equal(tok, "+")) {
//             tok = tok->next;
//             continue;
//         }

//         break;        
//     }
    
//     node->lhs = primary(&tok, tok);
//     *rest = tok;
//     return head->lhs;
// }

static Node *primary(Token **rest, Token *tok) {
    Node *node = NULL;
    
    if (tok->type == TK_NUM) {
        node = new_num(get_number(tok));
        *rest = tok->next;
        return node;
    }

    if (equal(tok, "(")) {
        node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }

    error_tok(tok, "expected an expression");
}

//
// Code generator
//

static int depth;

static void push(void) {
    printf("  push %%rax\n");
    depth++;
}

static void pop(char *arg) {
    printf("  pop %s\n", arg);
    depth--;
}

static void gen_expr(Node *node) {
    switch (node->type) {
        case ND_NUM:
            printf("  mov $%d, %rax\n", node->val);
            return;
        case ND_NEG:
            gen_expr(node->lhs);
            printf("  neg %rax\n");
            return;
    }


    gen_expr(node->rhs);
    push();
    gen_expr(node->lhs);
    pop("%rdi");

    switch (node->type)
    {
    case ND_ADD:
        printf("  add %rdi, %rax\n");
        return;
    case ND_SUB:
        printf("  sub %rdi, %rax\n");
        return;
    case ND_MUL:
        printf("  imul %rdi, %rax\n");
        return;
    case ND_DIV:
        printf("  cqo\n");
        printf("  idiv %rdi, %rax\n");
        return;
    case ND_EQ:
    case ND_LE:
    case ND_NE:
    case ND_LT:
        printf("  cmp %%rdi, %%rax\n");

        if (node->type == ND_EQ)
            printf("  sete %%al\n");
        else if (node->type == ND_NE)
            printf("  setne %%al\n");
        else if (node->type == ND_LT)
            printf("  setl %%al\n");
        else if (node->type == ND_LE)
            printf("  setle %%al\n");

        printf("  movzb %%al, %%rax\n");
        return;
    }

    error("invalid expression");
}






int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    current_input = argv[1];
    Token *tok = tokenize();

    Node *e = expr(&tok, tok);

    printf("  .globl main\n");
    printf("main:\n");

    gen_expr(e);
    printf("  ret\n");
    
    assert(depth == 0);
    return 0;
}
