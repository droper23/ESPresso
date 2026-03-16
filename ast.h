// ast.h

#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef enum {
    NODE_NUMBER,
    NODE_FLOAT,
    NODE_STRING,
    NODE_NULL,
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
    NODE_FUNCTION_DEF,
    NODE_VAR_DECL,
    NODE_LOGICAL_AND,
    NODE_LOGICAL_OR,
    NODE_RANGE,
    NODE_AUGMENTED_ASSIGN,
    NODE_ARRAY,
    NODE_ARRAY_INDEX
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
    struct ASTNode* alternate; // For 'else' branches
    struct ASTNode* increment; // For old for loops (or custom use)
    struct ASTNode* next;

    TokenType tokenType;
    int isConst;   // For declarations
    char* loopVar; // For range loops

    int line;
    int column;
} ASTNode;

#endif
