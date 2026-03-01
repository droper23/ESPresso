#include "lexer.h"
#include <stdio.h>

int main(void) {
    const char* source = "print 5 + 3";
    const char* current = source;

    Token t = getNextToken(current);
    printf("Token type: %d, lexeme: %s\n", t.type, t.lexeme);
}