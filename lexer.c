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
    lexer->line = 1;
    lexer->column = 1;
}

bool matchNext(Lexer* lexer, char expected) {
    if (*lexer->current == '\0' || *lexer->current != expected)
        return false;

    lexer->current++;
    lexer->column++;
    return true;
}

Token getNextToken(Lexer* lexer) {

    while (1) {

        // skip whitespace
        while (*lexer->current && isspace(*lexer->current)) {
            if (*lexer->current == '\n') {
                lexer->line++;
                lexer->column = 1;
            } else {
                lexer->column++;
            }
            lexer->current++;
        }

        // skip # comments
        if (*lexer->current == '#') {
            while (*lexer->current && *lexer->current != '\n') {
                lexer->current++;
                lexer->column++;
            }
            continue;
        }

        // skip // comments
        if (*lexer->current == '/' && *(lexer->current + 1) == '/') {
            lexer->current += 2;
            lexer->column += 2;
            while (*lexer->current && *lexer->current != '\n') {
                lexer->current++;
                lexer->column++;
            }
            continue;
        }

        break;
    }

    lexer->start = lexer->current;
    char c = *lexer->current++;
    lexer->column++;

    Token t;
    t.line = lexer->line;
    t.column = lexer->column;

    if (c == '\0') {
        t.type = TOKEN_EOF;
        t.lexeme = "";
        return t;
    }

    // numbers
    if (isdigit(c)) {
        while (isdigit(*lexer->current)) {
            lexer->current++;
            lexer->column++;
        }

        if (*lexer->current == '.' && isdigit(*(lexer->current + 1))) {
            t.type = TOKEN_FLOAT;
            lexer->current++;
            lexer->column++;
            while (isdigit(*lexer->current)) {
                lexer->current++;
                lexer->column++;
            }
        } else {
            t.type = TOKEN_NUMBER;
        }

        int len = lexer->current - lexer->start;

        t.lexeme = strndup(lexer->start, len);
        return t;
    }

    // identifiers and keywords
    if (isalpha(c) || c == '_') {

        while (isalnum(*lexer->current) || *lexer->current == '_') {
            lexer->current++;
            lexer->column++;
        }

        int len = lexer->current - lexer->start;
        char* text = strndup(lexer->start, len);

        if (strcmp(text, "if") == 0)
            t.type = TOKEN_IF;
        else if (strcmp(text, "else") == 0)
            t.type = TOKEN_ELSE;
        else if (strcmp(text, "while") == 0)
            t.type = TOKEN_WHILE;
        else if (strcmp(text, "for") == 0)
            t.type = TOKEN_FOR;
        else if (strcmp(text, "in") == 0)
            t.type = TOKEN_IN;
        else if (strcmp(text, "fn") == 0)
            t.type = TOKEN_FN;
        else if (strcmp(text, "return") == 0)
            t.type = TOKEN_RETURN;
        else if (strcmp(text, "and") == 0)
            t.type = TOKEN_AND;
        else if (strcmp(text, "or") == 0)
            t.type = TOKEN_OR;
        else if (strcmp(text, "const") == 0)
            t.type = TOKEN_CONST;
        else if (strcmp(text, "var") == 0)
            t.type = TOKEN_VAR;
        else if (strcmp(text, "true") == 0)
            t.type = TOKEN_TRUE;
        else if (strcmp(text, "false") == 0)
            t.type = TOKEN_FALSE;
        else if (strcmp(text, "null") == 0)
            t.type = TOKEN_NULL;
        else
            t.type = TOKEN_IDENTIFIER;

        t.lexeme = text;
        return t;
    }

    // strings
    if (c == '"') {
        int capacity = 16;
        char* buffer = malloc(capacity);
        int length = 0;

        while (*lexer->current != '\0' && *lexer->current != '"') {
            char curr = *lexer->current;
            if (curr == '\\') {
                lexer->current++;
                lexer->column++;
                if (*lexer->current == 'n') curr = '\n';
                else if (*lexer->current == 'r') curr = '\r';
                else if (*lexer->current == 't') curr = '\t';
                else if (*lexer->current == '\\') curr = '\\';
                else if (*lexer->current == '"') curr = '"';
            }

            if (length + 1 >= capacity) {
                capacity *= 2;
                buffer = realloc(buffer, capacity);
            }
            buffer[length++] = curr;

            if (*lexer->current == '\n') {
                lexer->line++;
                lexer->column = 1;
            } else {
                lexer->column++;
            }
            lexer->current++;
        }

        if (*lexer->current != '"') {
            free(buffer);
            t.type = TOKEN_UNKNOWN;
            t.lexeme = "Unterminated string";
            return t;
        }
        buffer[length] = '\0';
        t.type = TOKEN_STRING;
        t.lexeme = buffer;

        lexer->current++; // skip closing quote
        lexer->column++;
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
            if (matchNext(lexer, '=')) {
                t.type = TOKEN_PLUS_EQUAL;
                t.lexeme = "+=";
            } else {
                t.type = TOKEN_PLUS;
                t.lexeme = "+";
            }
            return t;

        case '-':
            if (matchNext(lexer, '=')) {
                t.type = TOKEN_MINUS_EQUAL;
                t.lexeme = "-=";
            } else {
                t.type = TOKEN_MINUS;
                t.lexeme = "-";
            }
            return t;

        case '*':
            if (matchNext(lexer, '=')) {
                t.type = TOKEN_STAR_EQUAL;
                t.lexeme = "*=";
            } else {
                t.type = TOKEN_MULTIPLY;
                t.lexeme = "*";
            }
            return t;

        case '/':
            if (matchNext(lexer, '=')) {
                t.type = TOKEN_SLASH_EQUAL;
                t.lexeme = "/=";
            } else {
                t.type = TOKEN_DIVIDE;
                t.lexeme = "/";
            }
            return t;

        case '(':
            t.type = TOKEN_OPEN_PARENTHESIS;
            t.lexeme = "(";
            return t;

        case ')':
            t.type = TOKEN_CLOSE_PARENTHESIS;
            t.lexeme = ")";
            return t;

        case '[':
            t.type = TOKEN_OPEN_BRACKETS;
            t.lexeme = "[";
            return t;

        case ']':
            t.type = TOKEN_CLOSE_BRACKETS;
            t.lexeme = "]";
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

        case '&':
            if (matchNext(lexer, '&')) {
                t.type = TOKEN_AND;
                t.lexeme = "&&";
            } else {
                t.type = TOKEN_UNKNOWN;
                t.lexeme = "&";
            }
            return t;

        case '|':
            if (matchNext(lexer, '|')) {
                t.type = TOKEN_OR;
                t.lexeme = "||";
            } else {
                t.type = TOKEN_UNKNOWN;
                t.lexeme = "|";
            }
            return t;

        case '.':
            if (matchNext(lexer, '.')) {
                t.type = TOKEN_DOTDOT;
                t.lexeme = "..";
                return t;
            }
            t.type = TOKEN_UNKNOWN;
            t.lexeme = ".";
            return t;
        default:
            t.type = TOKEN_UNKNOWN;
            t.lexeme = strndup(&c, 1);
            return t;
    }
}