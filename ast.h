// ast.h

#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef enum {
    NODE_NUMBER,
    NODE_BINARY_OP,
    NODE_PRINT,
    NODE_IDENTIFIER,
    NODE_UNKNOWN
} NodeType;

typedef struct ASTNode {
    NodeType type;
    struct ASTNode* left;
    struct ASTNode* right;
    int value;
    char* name;
    char op;
    TokenType tokenType;
} ASTNode;

#endif
