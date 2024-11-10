#include "chibicc.h"
#include <assert.h>
#include <stdio.h>

//
// Code generator
//

int depth = 0;

int if_label_num = 0;

static void push(void) {
    printf("  push %%rax\n");
    depth++;
}

static void pop(char *arg) {
    printf("  pop %s\n", arg);
    depth--;
}

// Round up `n` to the nearest multiple of `align`. For instance,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16.
static int align_to(int n, int align) {
    return (n + align - 1) * align / align;
}

static void gen_addr(Node *node) {
    if (node->type == ND_VAR) {
        int offset = node->var->offset;
        printf("  lea %d(%%rbp), %%rax\n", offset);
    }
}

static void gen_expr(Node *node) {
    switch (node->type) {
    case ND_NUM:
        printf("  mov $%d, %%rax\n", node->val);
        return;
    case ND_NEG:
        gen_expr(node->lhs);
        printf("  neg %%rax\n");
        return;
    case ND_VAR:
        gen_addr(node);
        printf("  movq (%%rax), %%rax\n");
        return;
    }

    gen_expr(node->rhs);
    push();
    gen_expr(node->lhs);
    pop("%rdi");

    switch (node->type) {
    case ND_ADD:
        printf("  add %%rdi, %%rax\n");
        return;
    case ND_SUB:
        printf("  sub %%rdi, %%rax\n");
        return;
    case ND_MUL:
        printf("  imul %%rdi, %%rax\n");
        return;
    case ND_DIV:
        printf("  cqo\n");
        printf("  idiv %%rdi, %%rax\n");
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
    case ND_ASSIGN:
        gen_addr(node->lhs);
        printf("  movq %%rdi, (%%rax)\n");
        printf("  movq (%%rax), %%rax\n");
        return;
    }

    error("invalid expression");
}

static void gen_stmt(Node *node) {
    switch (node->type) {
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        break;
    case ND_RETURN_STMT:
        gen_expr(node->lhs);
        printf("  jmp .L.return\n");
        break;
    case ND_BLOCK:
        node = node->body;
        while (node != NULL) {
            gen_stmt(node);
            node = node->next;
        }
        break;
    case ND_IF:
        // node = node->next;
        gen_expr(node->lhs);
        printf("  cmpl $0, %%eax\n");
        int cur = if_label_num;
        if_label_num += 2;
        printf("  je .L%d\n", cur);
        gen_stmt(node->body);
        printf("  jmp .L%d\n", cur + 1);
        printf(".L%d:\n",cur);

        // else branch
        if (node->rhs != NULL) {
            gen_stmt(node->rhs);
        }

        printf(".L%d:\n", cur + 1);

        break;
    default:
        error("invalid statement");
    }
}

static void calc_local_variable_offset(Function *prog) {
    int offset = 0;
    for (Obj *local = prog->locals; local != NULL; local = local->next) {
        offset += 8;
        local->offset = -offset;

        printf("# offset: %d, %c\n", offset, *local->name);
    }

    // 16 byte stack alignment
    // https://stackoverflow.com/questions/49391001/why-does-the-x86-64-amd64-system-v-abi-mandate-a-16-byte-stack-alignment
    prog->stack_size = align_to(offset, 16);
}

void codegen(Function *prog) {
    // calculate offset for locals
    calc_local_variable_offset(prog);

    printf("  .globl main\n");
    printf("main:\n");

    printf("  pushq %%rbp\n");
    printf("  movq %%rsp, %%rbp\n");

    // reserve stack space according to local variables
    printf("  subq $%d, %%rsp\n", prog->stack_size);

    gen_stmt(prog->body);
    assert(depth == 0);

    printf(".L.return:\n");
    printf("  movq %%rbp, %%rsp\n");
    printf("  popq %%rbp\n");
    printf("  ret\n");
}
