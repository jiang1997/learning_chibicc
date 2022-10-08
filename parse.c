#include "chibicc.h"

//
// Parser
//

static Node *stmt(Token **rest, Token *tok);
static Node *expr_stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

static int get_number(Token *tok) {
    if (tok->type != TK_NUM) {
        error_tok(tok, "expected a number");
    }
    return tok->val;
}

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

static Node *new_unary(NodeType type, Node *expr) {
    Node *node = new_node(type);
    node->lhs = expr;
    return node;
}

/*
stmt       = expr-stmt
expr-stmt  = expt ";"
expr       = equality
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? primary
primary    = num | "(" expr ")"
*/


Node *parse(Token *tok) {
    Node head = {};
    Node *cur = &head;
    while (tok->type != TK_EOF) {
        cur = cur->next = stmt(&tok, tok);
    }
    return head.next;
}

static Node *stmt(Token **rest, Token *tok) {
    return expr_stmt(rest, tok);
}

static Node *expr_stmt(Token **rest, Token *tok) {
    Node *node = new_unary(ND_EXPR_STMT, expr(&tok, tok));
    *rest = skip(tok, ";");
    return node;
}

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
        return new_unary(ND_NEG, unary(rest, tok->next));
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
