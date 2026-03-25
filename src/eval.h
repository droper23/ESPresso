// eval.h
// Created by Derek Roper on 3/2/26.
//

#ifndef EVAL_H
#define EVAL_H
#include "ast.h"
#include "env.h"
#include "value.h"

Value evaluate(ASTNode* node, Env* env);

void freeAST(ASTNode* node);
void evalSetSource(const char* source, const char* path);

#endif //EVAL_H
