#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    const char* source = "a == b != c >= 5 < 10";
    Lexer lexer;
    lexer.current = source;

    Token t;
    do {
        t = getNextToken(&lexer);
        printf("Token type: %s, lexeme: '%s'\n", tokenTypeToString(t.type), t.lexeme);

        if (t.type == TOKEN_NUMBER || t.type == TOKEN_IDENTIFIER) {
            free(t.lexeme);
        }

    } while (t.type != TOKEN_EOF);

    return 0;
}