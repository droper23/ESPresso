// chunk.h
// Created by Derek Roper on 3/16/26.
//

#ifndef chunk_H
#define chunk_H

#include <stdint.h>
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_NULL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_INDEX,
    OP_SET_INDEX,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_ARRAY,
    OP_RANGE,
    OP_RANGE_START,
    OP_RANGE_END,
    OP_CLOSURE,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_CLOSE_UPVALUE,
    OP_RETURN,
} OpCode;

typedef struct Chunk {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    int* columns;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line, int column);
int addConstant(Chunk* chunk, Value value);

#endif // chunk_H
