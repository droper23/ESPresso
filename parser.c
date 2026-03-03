// parser.c

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

ASTNode* makeNode(NodeType type) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = type;
    node->left = node->right = NULL;
    node->value = 0;
    node->name = NULL;
    node->op = 0;
    node->next = NULL;
    return node;
}

ASTNode* makeUnknownNode(Parser* parser) {
    ASTNode* node = makeNode(NODE_UNKNOWN);

    if (parser->current.lexeme && parser->current.lexeme[0] != '\0') {
        node->name = strdup(parser->current.lexeme);
    } else {
        node->name = strdup("");
    }

    advance(parser);

    while (parser->current.type == TOKEN_UNKNOWN) {
        advance(parser);
    }

    return node;
}

ASTNode* parseAdditive(Parser* parser);
ASTNode* parseTerm(Parser* parser);
ASTNode* parseFactor(Parser* parser);

ASTNode* parseStatement(Parser* parser) {
    TokenType t = parser->current.type;

    if (t == TOKEN_PRINT) {
        advance(parser);
        ASTNode* expr = parseExpression(parser);
        ASTNode* node = makeNode(NODE_PRINT);
        node->left = expr;
        return node;
    }

    if (t == TOKEN_IDENTIFIER) {
        ASTNode* node = parseAssignment(parser);
        return node;
    }

    if (t == TOKEN_OPEN_BRACES) {
        advance(parser);
        ASTNode* block = makeNode(NODE_BLOCK);
        ASTNode* last = NULL;

        while (parser->current.type != TOKEN_CLOSE_BRACES && parser->current.type != TOKEN_EOF) {
            ASTNode* stmt = parseStatement(parser);
            if (!block->body) block->body = stmt;
            else last->next = stmt;
            last = stmt;
        }

        if (parser->current.type != TOKEN_CLOSE_BRACES) {
            printf("Error: expected '}', found '%s'\n", parser->current.lexeme);
        } else {
            advance(parser);
        }

        return block;
    }

    printf("Error: unexpected token '%s'\n", parser->current.lexeme);
    return makeUnknownNode(parser);
}

ASTNode* parseExpression(Parser* parser) {
    return parseAssignment(parser);
}

ASTNode* parseAssignment(Parser* parser)
{
    ASTNode* left = parseAdditive(parser);

    if (parser->current.type == TOKEN_EQUAL) {
        advance(parser);
        if (left->type == NODE_IDENTIFIER) {
            ASTNode* right = parseAssignment(parser);
            ASTNode* node = makeNode(NODE_ASSIGN);
            node->left = left;
            node->right = right;
            return node;
        }
        printf("Error: Invalid assignment target, found '%s'\n", parser->current.lexeme);
        return makeUnknownNode(parser);
    }

    return left;
}

ASTNode* parseAdditive(Parser* parser) {
    ASTNode* left = parseTerm(parser);

    while (parser->current.type == TOKEN_PLUS || parser->current.type == TOKEN_MINUS) {
        char op = parser->current.lexeme[0];
        advance(parser);

        ASTNode* right = parseTerm(parser);

        ASTNode* node = makeNode(NODE_BINARY_OP);
        node->left = left;
        node->right = right;
        node->op = op;

        left = node;
    }
    return left;
}

ASTNode* parseTerm(Parser* parser) {
    ASTNode* left = parseFactor(parser);

    while (parser->current.type == TOKEN_STAR || parser->current.type == TOKEN_MULTIPLY) {
        char op = '*';
        advance(parser);

        ASTNode* right = parseFactor(parser);

        ASTNode* node = makeNode(NODE_BINARY_OP);
        node->left = left;
        node->right = right;
        node->op = op;

        left = node;
    }

    return left;
}

ASTNode* parseFactor(Parser* parser) {
    TokenType type = parser->current.type;

    if (type == TOKEN_NUMBER) {
        ASTNode* node = makeNode(NODE_NUMBER);
        node->value = atoi(parser->current.lexeme);
        advance(parser);
        return node;
    }
    else if (type == TOKEN_IDENTIFIER) {
        ASTNode* node = makeNode(NODE_IDENTIFIER);
        node->name = strdup(parser->current.lexeme);
        advance(parser);
        return node;
    }
    else if (type == TOKEN_OPEN_PARENTHESIS) {
        advance(parser);
        ASTNode* node = parseAssignment(parser);

        if (parser->current.type != TOKEN_CLOSE_PARENTHESIS) {
            printf("Error: expected ')', found '%s'\n", parser->current.lexeme);
            ASTNode* err = makeNode(NODE_UNKNOWN);
            err->name = strdup("missing ')'");
            err->left = node;
            return err;
        }

        advance(parser);
        return node;
    }
    else if (parser->current.type == TOKEN_EOF) {
        return NULL;
    } else {
        printf("Warning: unexpected token '%s'\n", parser->current.lexeme);

        ASTNode* unknown = makeUnknownNode(parser);

        if (parser->current.type == TOKEN_OPEN_PARENTHESIS) {
            advance(parser);

            ASTNode* inner = parseExpression(parser);

            if (parser->current.type != TOKEN_CLOSE_PARENTHESIS) {
                printf("Error: expected ')', found '%s'\n", parser->current.lexeme);
                ASTNode* err = makeNode(NODE_UNKNOWN);
                err->name = strdup("missing ')'");
                err->left = inner;
                unknown->left = err;
                return unknown;
            }

            advance(parser);
            unknown->left = inner;
        }

        return unknown;
    }

}