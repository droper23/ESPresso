// lexer.h
// Created by Derek Roper on 3/1/26.
//

#ifndef LEXER_H
#define LEXER_H
#include <stdbool.h>

#define TOKEN_LIST \
    X(TOKEN_PRINT) \
    X(TOKEN_NUMBER) \
    X(TOKEN_FLOAT) \
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
    X(TOKEN_MULTIPLY) \
    X(TOKEN_OPEN_PARENTHESIS) \
    X(TOKEN_CLOSE_PARENTHESIS) \
    X(TOKEN_OPEN_BRACKETS) \
    X(TOKEN_CLOSE_BRACKETS) \
    X(TOKEN_OPEN_BRACES) \
    X(TOKEN_CLOSE_BRACES) \
    X(TOKEN_COMMA) \
    X(TOKEN_PLUS_EQUAL) \
    X(TOKEN_MINUS_EQUAL) \
    X(TOKEN_IF) \
    X(TOKEN_WHILE) \
    X(TOKEN_DIVIDE) \
    X(TOKEN_STAR_EQUAL) \
    X(TOKEN_SLASH_EQUAL) \
    X(TOKEN_STRING) \
    X(TOKEN_SEMICOLON) \
    X(TOKEN_FN) \
    X(TOKEN_RETURN) \
    X(TOKEN_BREAK) \
    X(TOKEN_CONTINUE) \
    X(TOKEN_ELSE) \
    X(TOKEN_FOR) \
    X(TOKEN_IN) \
    X(TOKEN_CONST) \
    X(TOKEN_VAR) \
    X(TOKEN_TRUE) \
    X(TOKEN_FALSE) \
    X(TOKEN_AND) \
    X(TOKEN_OR) \
    X(TOKEN_NULL) \
    X(TOKEN_DOTDOT)

typedef enum {
#define X(name) name,
    TOKEN_LIST
#undef X
} TokenType;

typedef struct {
    const char* start;
    const char* current;
    int line;
    int column;
} Lexer;

void initLexer(Lexer* lexer, const char* input);

typedef struct Token {
    TokenType type;
    char* lexeme;
    // Lexemes are owned when ownsLexeme is true and must be freed via freeToken.
    bool ownsLexeme;
    int line;
    int column;
} Token;

Token getNextToken(Lexer* lexer);

void freeToken(Token* token);

bool matchNext(Lexer* lexer, char expected);

const char* tokenTypeToString(TokenType type);

#endif
