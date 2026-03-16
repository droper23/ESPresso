// debug.h
// Created by Derek Roper on 3/16/26.
//

#ifndef ESPRESSO_DEBUG_H
#define ESPRESSO_DEBUG_H

#include "chunk.h"

void disassembleChunk(Chunk* chunk, const char* name);
int disassembleInstruction(Chunk* chunk, int offset);

#endif
