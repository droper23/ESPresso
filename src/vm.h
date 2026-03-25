// vm.h
// Created by Derek Roper on 3/16/26.
//

#ifndef ESPRESSO_VM_H
#define ESPRESSO_VM_H

#include "chunk.h"
#include "env.h"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

InterpretResult interpret(Chunk* chunk, Env* globals);
void vmSetTrace(int enabled);
void vmFree(void);
void vmSetSource(const char* source, const char* path);

#endif // ESPRESSO_VM_H
