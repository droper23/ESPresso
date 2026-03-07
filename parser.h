// parser.h

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer* lexer;
    Token current;
} Parser;


/* core parser control */

void initParser(Parser* parser, Lexer* lexer);
void advance(Parser* parser);


/* AST helpers */

ASTNode* makeNode(NodeType type);
ASTNode* makeUnknownNode(Parser* parser);


/* parsing */

ASTNode* parseStatement(Parser* parser);

ASTNode* parseExpression(Parser* parser);
ASTNode* parseAssignment(Parser* parser);
ASTNode* parseExpressionPrecedence(Parser* parser, int precedence);

ASTNode* parseFactor(Parser* parser);


/* operator precedence */

int getPrecedence(TokenType type);

#endif