#include <assert.h>
#include "chibicc.h"

//
// Code generator
//

int depth = 0;

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


void codegen(Node *node) {
    printf("  .globl main\n");
    printf("main:\n");

    gen_expr(node);
    printf("  ret\n");

    assert(depth == 0);
}