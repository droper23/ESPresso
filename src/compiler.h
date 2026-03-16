// compiler.h
// Created by Derek Roper on 3/16/26.
//

#ifndef ESPRESSO_COMPILER_H
#define ESPRESSO_COMPILER_H

#include "ast.h"
#include "chunk.h"
#include <stdbool.h>

bool compile(ASTNode* program, Chunk* outChunk);

#endif //ESPRESSO_COMPILER_H
