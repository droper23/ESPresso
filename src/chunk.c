// chunk.c
// Created by Derek Roper on 3/16/26.
//

#include <stdlib.h>
#include <stdio.h>
#include "chunk.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    free(chunk->code);
    free(chunk->lines);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = oldCapacity < 8 ? 8 : oldCapacity * 2;
        chunk->code = realloc(chunk->code, sizeof(uint8_t) * chunk->capacity);
        chunk->lines = realloc(chunk->lines, sizeof(int) * chunk->capacity);
        if (chunk->code == NULL || chunk->lines == NULL) {
            perror("realloc");
            exit(1);
        }
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}
