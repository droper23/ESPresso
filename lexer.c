//
// Created by Derek Roper on 3/1/26.
//
#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

bool matchNext(Lexer* lexer, char expected) {
    if (*lexer->current != expected) {
        return false;
    }
    lexer->current++;

    return true;
}

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

    if (isalpha(c)) {
        while (isalnum(*lexer->current)) {
            lexer->current++;
        }
        int length = lexer->current - lexer->start;
        char* text = strndup(lexer->start, length);

        Token t;

        if (strcmp(text, "print") == 0) {
            t.type = TOKEN_PRINT;
            t.lexeme = "print";
        } else if (strcmp(text, "if") == 0) {
            t.type = TOKEN_IF;
            t.lexeme = "if";
        } else if (strcmp(text, "while") == 0) {
            t.type = TOKEN_WHILE;
            t.lexeme = "while";
        } else {
            t.type = TOKEN_IDENTIFIER;
            t.lexeme = text;
        }

        return t;
    }

    if (!isalnum(c)) {
        Token t;

        switch (c) {
            case '=':
                if (matchNext(lexer, '=')) {
                    t.type = TOKEN_EQUAL_EQUAL;
                    t.lexeme = "==";
                } else {
                    t.type = TOKEN_EQUAL;
                    t.lexeme = "=";
                }
                break;

            case '!':
                if (matchNext(lexer, '=')) {
                    t.type = TOKEN_NOT_EQUAL;
                    t.lexeme = "!=";
                } else {
                    t.type = TOKEN_NOT;
                    t.lexeme = "!";
                }
                break;

            case '+':
                if (matchNext(lexer, '=')) {
                    t.type = TOKEN_PLUS_EQUAL;
                    t.lexeme = "+=";
                } else {
                    t.type = TOKEN_PLUS;
                    t.lexeme = "+";
                }
                break;

            case '-':
                if (matchNext(lexer, '=')) {
                    t.type = TOKEN_MINUS_EQUAL;
                    t.lexeme = "-=";
                } else {
                    t.type = TOKEN_MINUS;
                    t.lexeme = "-";
                }
                break;

            case '>':
                if (matchNext(lexer, '=')) {
                    t.type = TOKEN_GREATER_EQUAL;
                    t.lexeme = ">=";
                } else {
                    t.type = TOKEN_GREATER;
                    t.lexeme = ">";
                }
                break;

            case '<':
                if (matchNext(lexer, '=')) {
                    t.type = TOKEN_LESS_EQUAL;
                    t.lexeme = "<=";
                } else {
                    t.type = TOKEN_LESS;
                    t.lexeme = "<";
                }
                break;

            case '(':
                t.type = TOKEN_OPEN_PARENTHESIS;
                t.lexeme = "(";
                break;

            case ')':
                t.type = TOKEN_CLOSE_PARENTHESIS;
                t.lexeme = ")";
                break;

            case '[':
                t.type = TOKEN_OPEN_BRACKETS;
                t.lexeme = "[";
                break;

            case ']':
                t.type = TOKEN_CLOSE_BRACKETS;
                t.lexeme = "]";
                break;

            case '{':
                t.type = TOKEN_OPEN_BRACES;
                t.lexeme = "{";
                break;

            case '}':
                t.type = TOKEN_CLOSE_BRACES;
                t.lexeme = "}";
                break;

            default:
                t.type = TOKEN_UNKNOWN;
                t.lexeme = "";
                lexer->current++;
                break;
        }

        return t;
    }
}


const char* tokenTypeToString(TokenType type) {
    switch(type) {
#define X(name) case name: return #name;
        TOKEN_LIST
#undef X
        default: return "UNKNOWN_TOKEN_TYPE";
    }
}