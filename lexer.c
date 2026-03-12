// lexer.c
// Created by Derek Roper on 3/1/26.
//

#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

void initLexer(Lexer* lexer, const char* input) {
    lexer->current = input;
    lexer->start = input;
}

bool matchNext(Lexer* lexer, char expected) {
    if (*lexer->current == '\0' || *lexer->current != expected)
        return false;

    lexer->current++;
    return true;
}

Token getNextToken(Lexer* lexer) {

    while (1) {

        // skip whitespace
        while (*lexer->current && isspace(*lexer->current))
            lexer->current++;

        // skip # comments
        if (*lexer->current == '#') {
            while (*lexer->current && *lexer->current != '\n')
                lexer->current++;
            continue;
        }

        // skip // comments
        if (*lexer->current == '/' && *(lexer->current + 1) == '/') {
            lexer->current += 2;
            while (*lexer->current && *lexer->current != '\n')
                lexer->current++;
            continue;
        }

        break;
    }

    lexer->start = lexer->current;
    char c = *lexer->current++;

    Token t;

    if (c == '\0') {
        t.type = TOKEN_EOF;
        t.lexeme = "";
        return t;
    }

    // numbers
    if (isdigit(c)) {
        while (isdigit(*lexer->current))
            lexer->current++;

        int len = lexer->current - lexer->start;

        t.type = TOKEN_NUMBER;
        t.lexeme = strndup(lexer->start, len);
        return t;
    }

    // identifiers and keywords
    if (isalpha(c) || c == '_') {

        while (isalnum(*lexer->current) || *lexer->current == '_')
            lexer->current++;

        int len = lexer->current - lexer->start;
        char* text = strndup(lexer->start, len);

        if (strcmp(text, "if") == 0)
            t.type = TOKEN_IF;
        else if (strcmp(text, "while") == 0)
            t.type = TOKEN_WHILE;
        else
            t.type = TOKEN_IDENTIFIER;

        t.lexeme = text;
        return t;
    }

    // strings
    if (c == '"') {

        const char* strStart = lexer->current;

        while (*lexer->current && *lexer->current != '"')
            lexer->current++;

        if (*lexer->current != '"') {
            t.type = TOKEN_UNKNOWN;
            t.lexeme = "Unterminated string";
            return t;
        }

        int len = lexer->current - strStart;

        t.type = TOKEN_STRING;
        t.lexeme = strndup(strStart, len);

        lexer->current++; // skip closing quote
        return t;
    }

    // operators and symbols
    switch (c) {

        case '=': {
            bool eq = matchNext(lexer, '=');
            t.type = eq ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL;
            t.lexeme = eq ? "==" : "=";
            return t;
        }

        case '!': {
            bool neq = matchNext(lexer, '=');
            t.type = neq ? TOKEN_NOT_EQUAL : TOKEN_NOT;
            t.lexeme = neq ? "!=" : "!";
            return t;
        }

        case '>': {
            if (matchNext(lexer, '=')) {
                t.type = TOKEN_GREATER_EQUAL;
                t.lexeme = ">=";
            } else {
                t.type = TOKEN_GREATER;
                t.lexeme = ">";
            }
            return t;
        }

        case '<': {
            if (matchNext(lexer, '=')) {
                t.type = TOKEN_LESS_EQUAL;
                t.lexeme = "<=";
            } else {
                t.type = TOKEN_LESS;
                t.lexeme = "<";
            }
            return t;
        }

        case '+':
            t.type = TOKEN_PLUS;
            t.lexeme = "+";
            return t;

        case '-':
            t.type = TOKEN_MINUS;
            t.lexeme = "-";
            return t;

        case '*':
            t.type = TOKEN_MULTIPLY;
            t.lexeme = "*";
            return t;

        case '/':
            t.type = TOKEN_DIVIDE;
            t.lexeme = "/";
            return t;

        case '(':
            t.type = TOKEN_OPEN_PARENTHESIS;
            t.lexeme = "(";
            return t;

        case ')':
            t.type = TOKEN_CLOSE_PARENTHESIS;
            t.lexeme = ")";
            return t;

        case '{':
            t.type = TOKEN_OPEN_BRACES;
            t.lexeme = "{";
            return t;

        case '}':
            t.type = TOKEN_CLOSE_BRACES;
            t.lexeme = "}";
            return t;

        case ',':
            t.type = TOKEN_COMMA;
            t.lexeme = ",";
            return t;

        case ';':
            t.type = TOKEN_SEMICOLON;
            t.lexeme = ";";
            return t;

        default:
            t.type = TOKEN_UNKNOWN;
            t.lexeme = strndup(&c, 1);
            return t;
    }
}