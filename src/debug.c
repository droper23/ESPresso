// debug.c
// Created by Derek Roper on 3/16/26.
//

#include <stdio.h>
#include "debug.h"
#include "object.h"
#include "value.h"

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

static int byteInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int closureInstruction(Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", "OP_CLOSURE", constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");

    Value constantValue = chunk->constants.values[constant];
    if (constantValue.type != VALUE_FUNCTION_OBJ) {
        return offset + 2;
    }

    FunctionObj* fn = constantValue.data.functionObj;
    int o = offset + 2;
    for (int i = 0; i < fn->upvalueCount; i++) {
        uint8_t isLocal = chunk->code[o++];
        uint8_t index = chunk->code[o++];
        printf("%04d   |                     %s %d\n", o - 2, isLocal ? "local" : "upvalue", index);
    }
    return o;
}

static int jumpInstruction(const char* name, int sign, Chunk* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static int globalInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t global_index = chunk->code[offset + 1];
    printf("%-16s %4d '", name, global_index);
    // The constant table stores the variable name
    printValue(chunk->constants.values[global_index]);
    printf("'\n");
    return offset + 2;
}

int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1] &&
        chunk->columns[offset] == chunk->columns[offset - 1]) {
        printf("   | ");
    } else {
        int line = chunk->lines[offset];
        int col = chunk->columns[offset];
        if (col > 0) {
            printf("%4d:%-3d ", line, col);
        } else {
            printf("%4d ", line);
        }
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_NULL:           return simpleInstruction("OP_NULL", offset);
        case OP_TRUE:           return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:          return simpleInstruction("OP_FALSE", offset);
        case OP_POP:            return simpleInstruction("OP_POP", offset);
        case OP_GET_LOCAL:      return byteInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:      return byteInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_GET_GLOBAL:     return globalInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL:  return globalInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:     return globalInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_GET_INDEX:      return simpleInstruction("OP_GET_INDEX", offset);
        case OP_SET_INDEX:      return simpleInstruction("OP_SET_INDEX", offset);
        case OP_EQUAL:          return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:        return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:           return simpleInstruction("OP_LESS", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_NOT:            return simpleInstruction("OP_NOT", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_PRINT:          return simpleInstruction("OP_PRINT", offset);
        case OP_CALL:           return byteInstruction("OP_CALL", chunk, offset);
        case OP_ARRAY:          return byteInstruction("OP_ARRAY", chunk, offset);
        case OP_RANGE:          return simpleInstruction("OP_RANGE", offset);
        case OP_RANGE_START:    return simpleInstruction("OP_RANGE_START", offset);
        case OP_RANGE_END:      return simpleInstruction("OP_RANGE_END", offset);
        case OP_CLOSURE:        return closureInstruction(chunk, offset);
        case OP_GET_UPVALUE:    return byteInstruction("OP_GET_UPVALUE", chunk, offset);
        case OP_SET_UPVALUE:    return byteInstruction("OP_SET_UPVALUE", chunk, offset);
        case OP_CLOSE_UPVALUE:  return simpleInstruction("OP_CLOSE_UPVALUE", offset);
        case OP_JUMP:           return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:  return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_LOOP:           return jumpInstruction("OP_LOOP", -1, chunk, offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
