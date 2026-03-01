//
// Created by Derek Roper on 3/1/26.
//

#ifndef LEXER_H
#define LEXER_H

enum TokenType {
    TOKEN_PRINT,
    TOKEN_NUMBER,
    TOKEN_PLUS,
    TOKEN_EOF,
    TOKEN_UNKNOWN,
    TOKEN_MINUS
};

typedef struct {
    const char* start;
    const char* current;
} Lexer;

void initLexer(Lexer* lexer, const char* source);

typedef struct Token {
    enum TokenType type;
    char* lexeme;
} Token;

Token getNextToken(Lexer* lexer);

#endif