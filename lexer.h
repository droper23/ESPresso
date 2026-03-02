//
// Created by Derek Roper on 3/1/26.
//

#ifndef LEXER_H
#define LEXER_H
#include <stdbool.h>

#define TOKEN_LIST \
    X(TOKEN_PRINT) \
    X(TOKEN_NUMBER) \
    X(TOKEN_IDENTIFIER) \
    X(TOKEN_PLUS) \
    X(TOKEN_MINUS) \
    X(TOKEN_EQUAL) \
    X(TOKEN_EQUAL_EQUAL) \
    X(TOKEN_NOT) \
    X(TOKEN_NOT_EQUAL) \
    X(TOKEN_GREATER) \
    X(TOKEN_GREATER_EQUAL) \
    X(TOKEN_LESS) \
    X(TOKEN_LESS_EQUAL) \
    X(TOKEN_EOF) \
    X(TOKEN_UNKNOWN) \
    X(TOKEN_STAR) \
    X(TOKEN_MULTIPLY) \
    X(TOKEN_OPEN_PARENTHESIS) \
    X(TOKEN_CLOSE_PARENTHESIS) \
    X(TOKEN_OPEN_BRACKETS) \
    X(TOKEN_CLOSE_BRACKETS) \
    X(TOKEN_OPEN_BRACES) \
    X(TOKEN_CLOSE_BRACES) \
    X(TOKEN_COMMA) \
    X(TOKEN_PLUS_EQUAL) \
    X(TOKEN_MINUS_EQUAL)

typedef enum {
#define X(name) name,
    TOKEN_LIST
#undef X
} TokenType;

typedef struct {
    const char* start;
    const char* current;
} Lexer;

void initLexer(Lexer* lexer, const char* source);

typedef struct Token {
    TokenType type;
    char* lexeme;
} Token;

Token getNextToken(Lexer* lexer);

bool matchNext(Lexer* lexer, char expected);

const char* tokenTypeToString(TokenType type);

#endif