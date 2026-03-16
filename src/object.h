// object.h
// Created by Derek Roper on 3/16/26.
//

#ifndef ESPRESSO_OBJECT_H
#define ESPRESSO_OBJECT_H

#include "value.h"
#include "chunk.h"

typedef enum {
    OBJ_FUNCTION,
    OBJ_STRING,
} ObjType;

typedef struct Obj {
    ObjType type;
    struct Obj* next;
} Obj;

typedef struct ObjString ObjString;

typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

typedef struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
} ObjString;

ObjFunction* newFunction();
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
void printObject(Value value);

#endif //ESPRESSO_OBJECT_H
