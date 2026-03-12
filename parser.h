// parser.h

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer* lexer;
    Token current;
} Parser;


void initParser(Parser* parser, Lexer* lexer);
void advance(Parser* parser);

ASTNode* makeNode(NodeType type);
ASTNode* makeUnknownNode(Parser* parser);

ASTNode* parseStatement(Parser* parser);

ASTNode* parseExpression(Parser* parser);
ASTNode* parseAssignment(Parser* parser);
ASTNode* parseExpressionPrecedence(Parser* parser, int precedence);

ASTNode* parseFactor(Parser* parser);

int getPrecedence(TokenType type);

#endif