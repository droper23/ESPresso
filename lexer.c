//
// Created by Derek Roper on 3/1/26.
//
#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

Token getNextToken(Lexer* lexer) {
    while (*lexer->current == ' ' || *lexer->current == '\t' || *lexer->current == '\n') {
        lexer->current++;
    }

    lexer->start = lexer->current;
    char c = *lexer->current++;

    if (c == '\0') {
        Token t;
        t.type = TOKEN_EOF;
        t.lexeme = "";
        return t;
    }

    if (isdigit(c)) {
        while (isdigit(*lexer->current)) {
            lexer->current++;
        }

        const int length = lexer->current - lexer->start;

        Token t;
        t.type = TOKEN_NUMBER;
        t.lexeme = strndup(lexer->start, length);
        return t;
    }

    if (strncmp(lexer->current-1, "print", 5) == 0) {
        char charAfterPrint = *(lexer->current - 1 + 5);
        if (!isalnum(charAfterPrint)) {
            lexer->current += 5;
            Token t;
            t.type = TOKEN_PRINT;
            t.lexeme = "print";
            return t;
        }
    }

    if (!isalnum(c)) {
        Token t;
        switch (c) {
            case '+':
                t.type = TOKEN_PLUS;
                t.lexeme = "+";
                break;
            case '-':
                t.type = TOKEN_MINUS;
                t.lexeme = "-";
                break;
            default:
                t.type = TOKEN_UNKNOWN;
                t.lexeme = "";
                lexer->current++;
        }
        return t;
    }
}
