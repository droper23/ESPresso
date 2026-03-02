#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void initParser(Parser* parser, Lexer* lexer) {
    parser->lexer = lexer;
    parser->current = getNextToken(lexer);
}

void advance(Parser* parser) {
    parser->current = getNextToken(parser->lexer);
}

ASTNode* parseStatement(Parser* parser) {
    if (parser->current.type == TOKEN_PRINT) {
        advance(parser);

        ASTNode* expr = parseExpression(parser);

        ASTNode* node = malloc(sizeof(ASTNode));
        node->type = NODE_PRINT;
        node->left = expr;
        node->right = NULL;
        return node;
    }

    printf("Error: expected 'print'\n");
    return NULL;
}

ASTNode* parseExpression(Parser* parser) {
    ASTNode* left = parseTerm(parser);

    while (parser->current.type == TOKEN_PLUS || parser->current.type == TOKEN_MINUS) {
        char op = parser->current.lexeme[0];
        advance(parser);

        ASTNode* right = parseTerm(parser);

        ASTNode* node = malloc(sizeof(ASTNode));
        node->type = NODE_BINARY_OP;
        node->left = left;
        node->right = right;
        node->op = op;

        left = node;
    }
    return left;
}

ASTNode* parseTerm(Parser* parser) {

    ASTNode* node = malloc(sizeof(ASTNode));

    if (parser->current.type == TOKEN_NUMBER) {
        node->type = NODE_NUMBER;
        node->value = atoi(parser->current.lexeme);
        node->left = node->right = NULL;
        advance(parser);
    } else if (parser->current.type == TOKEN_IDENTIFIER) {
        node->type = NODE_IDENTIFIER;
        node->name = strdup(parser->current.lexeme);
        node->left = node->right = NULL;
        advance(parser);
    } else if (parser->current.type == TOKEN_OPEN_PARENTHESIS) {
        advance(parser);
        free(node);
        node = parseExpression(parser);
        if (parser->current.type != TOKEN_CLOSE_PARENTHESIS) {
            printf("Error: expected ')'\n");
        } else {
            advance(parser);
        }
    } else {
        node->type = NODE_UNKNOWN;
        node->left = node->right = NULL;
        node->value = 0;
        advance(parser);
        return node;
    }

    return node;
}