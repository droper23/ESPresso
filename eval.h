// eval.h
// Created by Derek Roper on 3/2/26.
//

#ifndef EVAL_H
#define EVAL_H
#include "ast.h"
#include "env.h"

int evaluate(ASTNode* node, Env* env);

#endif //EVAL_H