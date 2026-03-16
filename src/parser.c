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
    node->line = 0;
    node->column = 0;
    return node;
}

ASTNode* makeUnknownNode(Parser* parser) {
    ASTNode* node = makeNode(NODE_UNKNOWN);
    node->line = parser->current.line;
    node->column = parser->current.column;

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

        case TOKEN_DOTDOT:
            return 8;
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

        ASTNode* node = makeNode(op.type == TOKEN_DOTDOT ? NODE_RANGE : NODE_BINARY_OP);
        node->line = op.line;
        node->column = op.column;
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

    TokenType t = parser->current.type;
    if (t == TOKEN_EQUAL || t == TOKEN_PLUS_EQUAL || t == TOKEN_MINUS_EQUAL || 
        t == TOKEN_STAR_EQUAL || t == TOKEN_SLASH_EQUAL) {

        Token opToken = parser->current;
        advance(parser);

        if (left->type == NODE_IDENTIFIER || left->type == NODE_ARRAY_INDEX) {
            ASTNode* right = parseAssignment(parser);

            NodeType nodeType = (opToken.type == TOKEN_EQUAL) ? NODE_ASSIGN : NODE_AUGMENTED_ASSIGN;
            ASTNode* node = makeNode(nodeType);
            node->line = left->line;
            node->column = left->column;
            node->left = left;
            node->right = right;
            
            if (nodeType == NODE_AUGMENTED_ASSIGN) {
                char* op = strdup(opToken.lexeme);
                op[1] = '\0';
                node->name = op;
            }

            return node;
        }

        printf("Error: Invalid assignment target at [Line %d, Col %d]\n", left->line, left->column);
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
    node->line = condition->line;
    node->column = condition->column;
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

    int isConst = 0;
    if (parser->current.type == TOKEN_CONST || parser->current.type == TOKEN_VAR) {
        isConst = (parser->current.type == TOKEN_CONST);
        advance(parser);
    }

    if (parser->current.type != TOKEN_IDENTIFIER) {
        printf("Error: expected identifier in for loop\n");
        return makeUnknownNode(parser);
    }

    char* loopVar = strdup(parser->current.lexeme);
    advance(parser);

    if (parser->current.type != TOKEN_IN) {
        printf("Error: expected 'in' in for loop\n");
        free(loopVar);
        return makeUnknownNode(parser);
    }
    advance(parser);

    ASTNode* rangeExpr = parseExpression(parser);

    if (parser->current.type != TOKEN_OPEN_BRACES) {
        printf("Error: expected '{' after for loop header\n");
        free(loopVar);
        return makeUnknownNode(parser);
    }

    ASTNode* body = parseStatement(parser);

    ASTNode* node = makeNode(NODE_FOR);
    node->loopVar = loopVar;
    node->left = rangeExpr;
    node->body = body;
    node->isConst = isConst;
    return node;
}

ASTNode* parseVarDecl(Parser* parser) {
    TokenType t = parser->current.type;
    advance(parser);

    if (parser->current.type != TOKEN_IDENTIFIER) {
        printf("Error: expected identifier after declaration keyword\n");
        return makeUnknownNode(parser);
    }

    ASTNode* id = makeNode(NODE_IDENTIFIER);
    id->line = parser->current.line;
    id->column = parser->current.column;
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
    node->line = body->line;
    node->column = body->column;
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
    node->line = (expr) ? expr->line : parser->current.line;
    node->column = (expr) ? expr->column : parser->current.column;
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
    node->line = condition->line;
    node->column = condition->column;
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

    if (t == TOKEN_CONST || t == TOKEN_VAR) {
        ASTNode* node = parseVarDecl(parser);
        if (parser->current.type == TOKEN_SEMICOLON) advance(parser);
        return node;
    }

    if (t == TOKEN_PRINT) {
        advance(parser);
        ASTNode* expr = parseExpression(parser);

        if (parser->current.type == TOKEN_SEMICOLON) advance(parser);

        ASTNode* node = makeNode(NODE_PRINT);
        node->line = expr->line;
        node->column = expr->column;
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

    if (type == TOKEN_NUMBER || type == TOKEN_FLOAT) {

        ASTNode* node = makeNode(NODE_NUMBER);
        node->line = parser->current.line;
        node->column = parser->current.column;
        node->tokenType = type;
        node->floatValue = atof(parser->current.lexeme);
        node->value = (int)node->floatValue;

        advance(parser);
        return node;
    }

    if (type == TOKEN_STRING) {
        char* lexeme = parser->current.lexeme;
        char* interpolated = strstr(lexeme, "{");
        
        if (interpolated) {
            ASTNode* root = NULL;
            char* start = lexeme;
            
            while ((interpolated = strstr(start, "{"))) {
                // Part before {
                int len = interpolated - start;
                if (len > 0) {
                    ASTNode* part = makeNode(NODE_STRING);
                    part->name = strndup(start, len);
                    if (!root) root = part;
                    else {
                        ASTNode* add = makeNode(NODE_BINARY_OP);
                        add->name = strdup("+");
                        add->left = root;
                        add->right = part;
                        root = add;
                    }
                }
                
                // Content inside { }
                start = interpolated + 1;
                char* end = strchr(start, '}');
                if (!end) {
                    printf("Error: Unterminated interpolation at [Line %d]\n", parser->current.line);
                    break;
                }
                
                int exprLen = end - start;
                char* exprStr = strndup(start, exprLen);
                
                // Temporary parser for the expression
                Lexer tempLex;
                initLexer(&tempLex, exprStr);
                Parser tempParser;
                initParser(&tempParser, &tempLex);
                ASTNode* exprNode = parseExpression(&tempParser);
                free(exprStr);
                
                if (!root) root = exprNode;
                else {
                    ASTNode* add = makeNode(NODE_BINARY_OP);
                    add->name = strdup("+");
                    add->left = root;
                    add->right = exprNode;
                    root = add;
                }
                
                start = end + 1;
            }
            
            // Final part of string
            if (*start != '\0') {
                ASTNode* part = makeNode(NODE_STRING);
                part->name = strdup(start);
                if (!root) root = part;
                else {
                    ASTNode* add = makeNode(NODE_BINARY_OP);
                    add->name = strdup("+");
                    add->left = root;
                    add->right = part;
                    root = add;
                }
            }
            
            advance(parser);
            return root ? root : makeNode(NODE_STRING);
        }

        ASTNode* node = makeNode(NODE_STRING);
        node->line = parser->current.line;
        node->column = parser->current.column;
        node->name = strdup(parser->current.lexeme);

        advance(parser);
        return node;
    }

    if (type == TOKEN_TRUE || type == TOKEN_FALSE) {
        ASTNode* node = makeNode(NODE_NUMBER);
        node->line = parser->current.line;
        node->column = parser->current.column;
        node->value = (type == TOKEN_TRUE) ? 1 : 0;
        node->tokenType = type;
        advance(parser);
        return node;
    }

    if (type == TOKEN_NULL) {
        ASTNode* node = makeNode(NODE_NULL);
        node->line = parser->current.line;
        node->column = parser->current.column;
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

    if (type == TOKEN_OPEN_BRACKETS) {
        ASTNode* node = makeNode(NODE_ARRAY);
        node->line = parser->current.line;
        node->column = parser->current.column;
        advance(parser);

        ASTNode* lastElem = NULL;
        while (parser->current.type != TOKEN_CLOSE_BRACKETS && parser->current.type != TOKEN_EOF) {
            ASTNode* elem = parseExpression(parser);
            if (!node->body) node->body = elem;
            else lastElem->next = elem;
            lastElem = elem;

            if (parser->current.type == TOKEN_COMMA) advance(parser);
        }

        if (parser->current.type == TOKEN_CLOSE_BRACKETS) advance(parser);
        else printf("Error: expected ']', found '%s'\n", parser->current.lexeme);

        while (parser->current.type == TOKEN_OPEN_BRACKETS) {
            advance(parser);
            ASTNode* index = parseExpression(parser);
            if (parser->current.type == TOKEN_CLOSE_BRACKETS) advance(parser);
            else printf("Error: expected ']', found '%s'\n", parser->current.lexeme);

            ASTNode* indexNode = makeNode(NODE_ARRAY_INDEX);
            indexNode->left = node; // The array
            indexNode->right = index; // The index
            node = indexNode;
        }

        return node;
    }

    if (type == TOKEN_IDENTIFIER) {
        ASTNode* node = makeNode(NODE_IDENTIFIER);
        node->line = parser->current.line;
        node->column = parser->current.column;
        node->name = strdup(parser->current.lexeme);
        advance(parser);

        while (true) {
            if (parser->current.type == TOKEN_OPEN_PARENTHESIS) {
                // Function call
                ASTNode* callNode = makeNode(NODE_FUNCTION);
                callNode->line = node->line;
                callNode->column = node->column;
                callNode->left = node; // The callee
                advance(parser); // Consume '('

                ASTNode* lastArg = NULL;
                if (parser->current.type != TOKEN_CLOSE_PARENTHESIS) {
                    while (true) {
                        ASTNode* arg = parseExpression(parser);
                        if (!callNode->body) callNode->body = arg;
                        else lastArg->next = arg;
                        lastArg = arg;

                        if (parser->current.type != TOKEN_COMMA) break;
                        advance(parser); // Consume ','
                    }
                }

                if (parser->current.type == TOKEN_CLOSE_PARENTHESIS) advance(parser);
                else printf("Error: expected ')' after arguments, found '%s'\n", parser->current.lexeme);

                node = callNode;
            } else if (parser->current.type == TOKEN_OPEN_BRACKETS) {
                // Array index
                advance(parser); // Consume '['
                ASTNode* index = parseExpression(parser);
                if (parser->current.type == TOKEN_CLOSE_BRACKETS) advance(parser);
                else printf("Error: expected ']' after index, found '%s'\n", parser->current.lexeme);

                ASTNode* indexNode = makeNode(NODE_ARRAY_INDEX);
                indexNode->left = node;   // The array being indexed
                indexNode->right = index; // The index expression
                node = indexNode;
            } else {
                break;
            }
        }
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