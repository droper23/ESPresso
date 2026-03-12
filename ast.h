// ast.h

#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef enum {
    NODE_NUMBER,
    NODE_STRING,
    NODE_BINARY_OP,
    NODE_PRINT,
    NODE_IDENTIFIER,
    NODE_UNKNOWN,
    NODE_BLOCK,
    NODE_ASSIGN,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_FUNCTION,
    NODE_RETURN,
} NodeType;

typedef struct ASTNode {
    NodeType type;
    struct ASTNode* left;
    struct ASTNode* right;

    int value;
    float floatValue;
    char* name;
    char op;

    struct ASTNode* body;
    struct ASTNode* next;

    TokenType tokenType;
} ASTNode;

#endif
