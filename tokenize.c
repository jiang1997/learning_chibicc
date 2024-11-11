#include "chibicc.h"

static char *current_input;

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

void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
}

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Consumes the current token if it matches `op`.
bool equal(Token *tok, char *s) {
    return memcmp(tok->loc, s, tok->len) == 0 && s[tok->len] == '\0';
}

// Ensure that the current token is `op`.
Token *skip(Token *tok, char *s) {
    if (!equal(tok, s)) {
        error_tok(tok, "expected '%s'", s);
    }
    return tok->next;
}

// Create a new token.
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

// Read a punctuator token from p and returns its length.
static int read_punct(char *p) {
    if (startwith(p, "==") || startwith(p, "!=") ||
        startwith(p, "<=") || startwith(p, ">="))
        return 2;
    
    return ispunct(*p) ? 1: 0;
}

static bool is_valid_ident_intial(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_valid_ident_subsequent(char c) {
    return is_valid_ident_intial(c) || (c >= '0' && c <= '9');
}

static bool is_keyword(Token *tok) {
    static char *key_words[] = {"return", "if", "else", "for"};

    for (int i = 0; i < sizeof(key_words) / sizeof(*key_words); ++i) {
        if (equal(tok, key_words[i]) ) {
            return true;
        }
    }

    return false;
}

static void remark_ident_as_keyword(Token *tok) {

    while(tok->type != TK_EOF) {
        if (is_keyword(tok)) {
            tok->type = TK_KEYWORD;
        }

        tok = tok->next;
    }
}

// Tokenize a given string and returns new tokens.
Token *tokenize(char *p) {
    current_input = p;
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

        // Identifier / Keyword
        if (is_valid_ident_intial(*p)) {
            char *start = p;

            while (is_valid_ident_subsequent(*p)) {
                p++;
            }

            cur = cur->next = new_token(TK_IDENT, start, p);
            continue;
        }
        
        error_at(p, "invalid token");
    }

    cur = cur->next = new_token(TK_EOF, p, p);
    remark_ident_as_keyword(head.next);

    return head.next;
}
