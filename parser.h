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
ASTNode* parseStatement(Parser* parser);
ASTNode* parseExpression(Parser* parser);
ASTNode* parseTerm(Parser* parser);

void advance(Parser* parser);

#endif