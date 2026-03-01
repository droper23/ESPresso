//
// Created by Derek Roper on 3/1/26.
//

#ifndef LEXER_H
#define LEXER_H

enum TokenType {
    TOKEN_PRINT,
    TOKEN_NUMBER,
    TOKEN_PLUS,
    TOKEN_EOF
};

typedef struct Token {
    enum TokenType type;
    char* lexeme;
};


#endif