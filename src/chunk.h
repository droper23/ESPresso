// chunk.h
// Created by Derek Roper on 3/16/26.
//

#ifndef chunk_H
#define chunk_H

#include <stdint.h>
#include "value.h"

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);

#endif // chunk_H