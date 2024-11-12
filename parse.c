#include "chibicc.h"
#include <stdio.h>
#include <string.h>

// All local variable instances created during parsing are accumulated to this
// list.
Obj *locals = NULL;

static Node *stmt(Token **rest, Token *tok);
static Node *return_stmt(Token **rest, Token *tok);
static Node *expr_stmt(Token **rest, Token *tok);
static Node *compound_stmt(Token **rest, Token *tok);
static Node *if_stmt(Token **rest, Token *tok);
static Node *while_stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);
static Node *for_stmt (Token **rest, Token *tok);

static Obj *find_local_var(Token *tok) {
    if (tok->type != TK_IDENT) {
        error_tok(tok, "expected a identifier");
    }

    Obj *tgt = locals;
    while (tgt != NULL && (tgt->length != tok->len ||
                           strncmp(tgt->name, tok->loc, tgt->length) != 0)) {
        tgt = tgt->next;
    }

    return tgt;
}

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

// static Node *new_var_node

static Node *new_unary(NodeType type, Node *expr) {
    Node *node = new_node(type);
    node->lhs = expr;
    return node;
}

static Node *new_var(Obj *var) {
    Node *node = new_node(ND_VAR);
    node->var = var;
    return node;
}

/*
stmt            = "return" expr";
                | compound-stmt
                | expr-stmt
                | if-stmt
                | for-stmt
                | while-stmt
while-stmt      = "while" "(" expr ")" stmt
for-stmt        = "for" "(" (expr)?  ";" (expr)? ";" (expr)? ")" stmt
compound_stmt = "{" stmt* "}"
if-stmt         = if-stmt ("else" stmt)?
null-stmt       = ";"
expr-stmt       = expr? ";"
expr            = assign
assign          = equality ("=" assign)?
equality        = relational ("==" relational | "!=" relational)*
relational      = add ("<" add | "<=" add | ">" add | ">=" add)*
add             = mul ("+" mul | "-" mul)*
mul             = unary ("*" unary | "/" unary)*
unary           = ("+" | "-")? primary
primary         = "(" expr ")" | ident | num
*/

Function *parse(Token *tok) {
    Node *body = compound_stmt(&tok, tok);

    Function *prog = calloc(1, sizeof(Function));
    prog->body = body;
    prog->locals = locals;

    return prog;
}


static Node *stmt(Token **rest, Token *tok) {
    if (equal(tok, "return")) {
        return return_stmt(rest, tok->next); 
    } else if(equal(tok, "{")) {
        return compound_stmt(rest, tok); 
    } else if(equal(tok, "if")) {
        return if_stmt(rest, tok);
    } else if(equal(tok, "for")) {
        return for_stmt(rest, tok->next);
    } else if (equal(tok, "while")) {
        return while_stmt(rest, tok->next);
    } else {
        return expr_stmt(rest, tok); 
    }
}

static Node *while_stmt(Token **rest, Token *tok) {
    Node *node = new_node(ND_FOR);

    tok = skip(tok, "(");
    node->for_cond = expr(&tok, tok);
    tok = skip(tok, ")");
    node->body = stmt(&tok, tok);

    *rest = tok;

    return node;
}


static Node *for_stmt(Token **rest, Token *tok) {
    Node *node = new_node(ND_FOR);

    tok = skip(tok, "(");
    if (!equal(tok, ";")) {
        node->for_init = expr(&tok, tok);
    }
    tok = skip(tok, ";");


    if (!equal(tok, ";")) {
        node->for_cond = expr(&tok, tok);
    }
    tok = skip(tok, ";");

    if (!equal(tok, ")")) {
        node->for_expr = expr(&tok, tok);
    }
    tok = skip(tok, ")");

    node->body = stmt(&tok, tok);

    *rest = tok;

    return node;
}

static Node *if_stmt(Token **rest, Token *tok) {
    tok = skip(tok, "if");

    tok = skip(tok, "(");

    Node *node = new_node(ND_IF);
    // condition
    node->cond = expr(&tok, tok);

    tok = skip(tok, ")");

    node->then = stmt(&tok, tok);

    if (equal(tok, "else")) {
        tok = tok->next;

        // differentiate two cases below by lhs
        node->els = stmt(&tok, tok);
    } 
    
    *rest = tok;

    return node;

}

static Node *compound_stmt(Token **rest, Token *tok) {
    tok = skip(tok, "{");

    Node *node = new_node(ND_BLOCK);
    Node **pre = &(node->body);


    while (tok->type != TK_EOF && !equal(tok, "}")) {
        Node *cur = stmt(&tok, tok);

        *pre = cur;
        pre = &(cur->next);
    }

    *rest = skip(tok, "}");

    return node;
}

static Node *return_stmt(Token **rest, Token *tok) {
    Node *node = new_unary(ND_RETURN_STMT, expr(&tok, tok));
    *rest = skip(tok, ";");
    return node;
}

static Node *expr_stmt(Token **rest, Token *tok) {
    if (equal(tok, ";")) {
        *rest = tok->next;
        return new_node(ND_BLOCK);
    }

    Node *node = new_unary(ND_EXPR_STMT, expr(&tok, tok));
    *rest = skip(tok, ";");
    return node;
}

static Node *expr(Token **rest, Token *tok) { 
    return assign(rest, tok);
}

static Node *assign(Token **rest, Token *tok) {
    Node *node = equality(&tok, tok);
    if (equal(tok, "=")) {
        node = new_binary(ND_ASSIGN, node, assign(&tok, tok->next));
    }
    *rest = tok;
    return node;
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

static Node *primary(Token **rest, Token *tok) {
    Node *node = NULL;

    if (tok->type == TK_NUM) {
        node = new_num(get_number(tok));
        *rest = tok->next;
        return node;
    }

    if (tok->type == TK_IDENT) {
        Obj *local_var = find_local_var(tok);
        if (local_var == NULL) {
            local_var = calloc(1, sizeof(Obj));
            local_var->length = tok->len;
            local_var->name = tok->loc;

            local_var->next = locals;
            locals = local_var;
        }

        node = new_var(local_var);
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
