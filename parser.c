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
    node->body = NULL;
    node->alternate = NULL;
    node->increment = NULL;
    return node;
}

ASTNode* makeUnknownNode(Parser* parser) {
    ASTNode* node = makeNode(NODE_UNKNOWN);

    if (parser->current.lexeme && parser->current.lexeme[0] != '\0')
        node->name = strdup(parser->current.lexeme);
    else
        node->name = strdup("");

    advance(parser);

    while (parser->current.type == TOKEN_UNKNOWN)
        advance(parser);

    return node;
}

int getPrecedence(TokenType type) {
    switch (type) {

        case TOKEN_OR:
            return 2;
        case TOKEN_AND:
            return 3;

        case TOKEN_EQUAL_EQUAL:
        case TOKEN_NOT_EQUAL:
            return 5;

        case TOKEN_GREATER:
        case TOKEN_LESS:
        case TOKEN_GREATER_EQUAL:
        case TOKEN_LESS_EQUAL:
            return 7;

        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return 10;

        case TOKEN_MULTIPLY:
        case TOKEN_DIVIDE:
            return 20;

        default:
            return 0;
    }
}

ASTNode* parseFactor(Parser* parser);
ASTNode* parseIf(Parser* parser);
ASTNode* parseWhile(Parser* parser);
ASTNode* parseFor(Parser* parser);
ASTNode* parseVarDecl(Parser* parser);
ASTNode* parseFunctionDef(Parser* parser);
ASTNode* parseReturn(Parser* parser);

ASTNode* parseExpressionPrecedence(Parser* parser, int precedence) {

    ASTNode* left = parseFactor(parser);

    while (1) {

        int opPrec = getPrecedence(parser->current.type);

        if (opPrec <= precedence)
            break;

        Token op = parser->current;
        advance(parser);

        ASTNode* right = parseExpressionPrecedence(parser, opPrec);

        ASTNode* node = makeNode(NODE_BINARY_OP);
        node->left = left;
        node->right = right;
        node->name = strdup(op.lexeme);

        left = node;
    }

    return left;
}

ASTNode* parseExpression(Parser* parser) {
    return parseExpressionPrecedence(parser, 0);
}

ASTNode* parseAssignment(Parser* parser) {

    ASTNode* left = parseExpressionPrecedence(parser, 0);

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

ASTNode* parseIf(Parser* parser) {

    advance(parser); // skip "if"

    ASTNode* condition = parseExpression(parser);

    if (parser->current.type != TOKEN_OPEN_BRACES) {
        printf("Error: expected '{' after if condition\n");
        return makeUnknownNode(parser);
    }

    ASTNode* body = parseStatement(parser);

    ASTNode* node = makeNode(NODE_IF);
    node->left = condition;
    node->body = body;

    if (parser->current.type == TOKEN_ELSE) {
        advance(parser);
        if (parser->current.type == TOKEN_IF) {
            node->alternate = parseIf(parser);
        } else {
            node->alternate = parseStatement(parser);
        }
    }

    return node;
}

ASTNode* parseFor(Parser* parser) {
    advance(parser); // skip 'for'

    ASTNode* init = NULL;
    if (parser->current.type == TOKEN_LET || parser->current.type == TOKEN_VAR) {
        init = parseVarDecl(parser);
    } else if (parser->current.type != TOKEN_SEMICOLON) {
        init = parseAssignment(parser);
    }
    if (parser->current.type == TOKEN_SEMICOLON) advance(parser);

    ASTNode* condition = NULL;
    if (parser->current.type != TOKEN_SEMICOLON) {
        condition = parseExpression(parser);
    }
    if (parser->current.type == TOKEN_SEMICOLON) advance(parser);

    ASTNode* increment = NULL;
    if (parser->current.type != TOKEN_OPEN_BRACES) {
        increment = parseAssignment(parser);
    }

    if (parser->current.type != TOKEN_OPEN_BRACES) {
        printf("Error: expected '{' after for header\n");
        return makeUnknownNode(parser);
    }

    ASTNode* body = parseStatement(parser);

    ASTNode* node = makeNode(NODE_FOR);
    node->left = init;
    node->right = condition;
    node->body = body;
    node->increment = increment;

    return node;
}

ASTNode* parseVarDecl(Parser* parser) {
    record_token_type:;
    TokenType t = parser->current.type; // TOKEN_LET or TOKEN_VAR
    advance(parser);

    if (parser->current.type != TOKEN_IDENTIFIER) {
        printf("Error: expected identifier after declaration keyword\n");
        return makeUnknownNode(parser);
    }

    ASTNode* id = makeNode(NODE_IDENTIFIER);
    id->name = strdup(parser->current.lexeme);
    advance(parser);

    ASTNode* init = NULL;
    if (parser->current.type == TOKEN_EQUAL) {
        advance(parser);
        init = parseExpression(parser);
    }

    ASTNode* node = makeNode(NODE_VAR_DECL);
    node->name = id->name;
    node->left = init;
    node->tokenType = t;

    return node;
}

ASTNode* parseFunctionDef(Parser* parser) {
    advance(parser); // skip 'fn'

    if (parser->current.type != TOKEN_IDENTIFIER) {
        printf("Error: expected identifier for function name\n");
        return makeUnknownNode(parser);
    }

    char* name = strdup(parser->current.lexeme);
    advance(parser);

    if (parser->current.type != TOKEN_OPEN_PARENTHESIS) {
        printf("Error: expected '(' after function name\n");
        return makeUnknownNode(parser);
    }
    advance(parser);

    ASTNode* params = NULL;
    ASTNode* lastParam = NULL;
    while (parser->current.type != TOKEN_CLOSE_PARENTHESIS && parser->current.type != TOKEN_EOF) {
        if (parser->current.type != TOKEN_IDENTIFIER) {
            printf("Error: expected parameter identifier\n");
            break;
        }

        ASTNode* param = makeNode(NODE_IDENTIFIER);
        param->name = strdup(parser->current.lexeme);
        advance(parser);

        if (!params) params = param;
        else lastParam->next = param;
        lastParam = param;

        if (parser->current.type == TOKEN_COMMA) advance(parser);
    }

    if (parser->current.type == TOKEN_CLOSE_PARENTHESIS) advance(parser);

    if (parser->current.type != TOKEN_OPEN_BRACES) {
        printf("Error: expected '{' for function body\n");
        return makeUnknownNode(parser);
    }

    ASTNode* body = parseStatement(parser);

    ASTNode* node = makeNode(NODE_FUNCTION_DEF);
    node->name = name;
    node->left = params;
    node->body = body;

    return node;
}

ASTNode* parseReturn(Parser* parser) {
    advance(parser); // skip 'return'

    ASTNode* expr = NULL;
    if (parser->current.type != TOKEN_SEMICOLON && parser->current.type != TOKEN_CLOSE_BRACES) {
        expr = parseExpression(parser);
    }

    ASTNode* node = makeNode(NODE_RETURN);
    node->left = expr;

    return node;
}

ASTNode* parseWhile(Parser* parser) {

    advance(parser); // skip "while"

    ASTNode* condition = parseExpression(parser);

    if (parser->current.type != TOKEN_OPEN_BRACES) {
        printf("Error: expected '{' after while condition\n");
        return makeUnknownNode(parser);
    }

    ASTNode* body = parseStatement(parser);

    ASTNode* node = makeNode(NODE_WHILE);
    node->left = condition;
    node->body = body;

    return node;
}

ASTNode* parseStatement(Parser* parser) {

    TokenType t = parser->current.type;

    if (t == TOKEN_IF)
        return parseIf(parser);

    if (t == TOKEN_WHILE)
        return parseWhile(parser);

    if (t == TOKEN_FOR)
        return parseFor(parser);

    if (t == TOKEN_FN)
        return parseFunctionDef(parser);

    if (t == TOKEN_RETURN) {
        ASTNode* node = parseReturn(parser);
        if (parser->current.type == TOKEN_SEMICOLON) advance(parser);
        return node;
    }

    if (t == TOKEN_LET || t == TOKEN_VAR) {
        ASTNode* node = parseVarDecl(parser);
        if (parser->current.type == TOKEN_SEMICOLON) advance(parser);
        return node;
    }

    if (t == TOKEN_PRINT) {
        advance(parser);
        ASTNode* expr = parseExpression(parser);

        if (parser->current.type == TOKEN_SEMICOLON) advance(parser);

        ASTNode* node = makeNode(NODE_PRINT);
        node->left = expr;

        return node;
    }

    if (t == TOKEN_IDENTIFIER) {
        ASTNode* node = parseAssignment(parser);
        if (parser->current.type == TOKEN_SEMICOLON) advance(parser);
        return node;
    }

    if (t == TOKEN_OPEN_BRACES) {

        advance(parser);

        ASTNode* block = makeNode(NODE_BLOCK);
        ASTNode* last = NULL;

        while (parser->current.type != TOKEN_CLOSE_BRACES && parser->current.type != TOKEN_EOF) {

            ASTNode* stmt = parseStatement(parser);

            if (!block->body)
                block->body = stmt;
            else
                last->next = stmt;

            last = stmt;
        }

        if (parser->current.type != TOKEN_CLOSE_BRACES)
            printf("Error: expected '}', found '%s'\n", parser->current.lexeme);
        else
            advance(parser);

        return block;
    }

    printf("Error: unexpected token '%s'\n", parser->current.lexeme);
    return makeUnknownNode(parser);
}

ASTNode* parseFactor(Parser* parser) {

    TokenType type = parser->current.type;

    if (type == TOKEN_NUMBER) {

        ASTNode* node = makeNode(NODE_NUMBER);
        node->value = atoi(parser->current.lexeme);

        advance(parser);
        return node;
    }

    if (type == TOKEN_STRING) {

        ASTNode* node = makeNode(NODE_STRING);
        node->name = strdup(parser->current.lexeme);

        advance(parser);
        return node;
    }

    if (type == TOKEN_TRUE || type == TOKEN_FALSE) {
        ASTNode* node = makeNode(NODE_NUMBER);
        node->value = (type == TOKEN_TRUE) ? 1 : 0;
        node->tokenType = type;
        advance(parser);
        return node;
    }

    if (type == TOKEN_NIL) {
        ASTNode* node = makeNode(NODE_NIL);
        advance(parser);
        return node;
    }

    if (type == TOKEN_NOT || type == TOKEN_MINUS) {
        Token op = parser->current;
        advance(parser);
        ASTNode* right = parseFactor(parser);
        ASTNode* node = makeNode(NODE_BINARY_OP);
        node->name = strdup(op.lexeme);
        node->right = right;
        return node;
    }

    if (type == TOKEN_IDENTIFIER) {

        Lexer savedLexer = *parser->lexer;
        Token next = getNextToken(parser->lexer);
        *parser->lexer = savedLexer;

        if (next.type == TOKEN_OPEN_PARENTHESIS) {

            ASTNode* node = makeNode(NODE_FUNCTION);
            node->name = strdup(parser->current.lexeme);

            advance(parser);
            advance(parser);

            ASTNode* lastArg = NULL;

            while (parser->current.type != TOKEN_CLOSE_PARENTHESIS && parser->current.type != TOKEN_EOF) {

                ASTNode* arg = parseExpression(parser);

                if (!node->body)
                    node->body = arg;
                else
                    lastArg->next = arg;

                lastArg = arg;

                if (parser->current.type == TOKEN_COMMA)
                    advance(parser);
            }

            if (parser->current.type == TOKEN_CLOSE_PARENTHESIS)
                advance(parser);
            else
                printf("Error: expected ')', found '%s'\n", parser->current.lexeme);

            return node;
        }

        ASTNode* node = makeNode(NODE_IDENTIFIER);
        node->name = strdup(parser->current.lexeme);

        advance(parser);
        return node;
    }

    if (type == TOKEN_OPEN_PARENTHESIS) {

        advance(parser);

        ASTNode* node = parseExpression(parser);

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

    if (type == TOKEN_EOF)
        return NULL;

    printf("Warning: unexpected token '%s'\n", parser->current.lexeme);
    return makeUnknownNode(parser);
}