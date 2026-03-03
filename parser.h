// parser.h

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer* lexer;
    Token current;
} Parser;

void advance(Parser* parser);
void initParser(Parser* parser, Lexer* lexer);
ASTNode* makeNode(NodeType type);
ASTNode* makeUnknownNode(Parser* parser);
ASTNode* parseExpression(Parser* parser);
ASTNode* parseAssignment(Parser* parser);
ASTNode* parseStatement(Parser* parser);
ASTNode* parseAdditive(Parser* parser);
ASTNode* parseTerm(Parser* parser);

#endif