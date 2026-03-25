// value.h
// Created by Derek Roper on 3/9/26.
//

#ifndef ESPRESSO_VALUE_H
#define ESPRESSO_VALUE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    VALUE_INT,
    VALUE_FLOAT,
    VALUE_STRING,
    VALUE_BOOL,
    VALUE_NULL,
    VALUE_FUNCTION,
    VALUE_FUNCTION_OBJ,
    VALUE_CLOSURE,
    VALUE_RETURN,
    VALUE_BREAK,
    VALUE_CONTINUE,
    VALUE_NATIVE,
    VALUE_RANGE,
    VALUE_ARRAY
} ValueType;

typedef enum {
    STRING_OWNERSHIP_NONE = 0,
    STRING_OWNERSHIP_VALUE = 1,
    STRING_OWNERSHIP_CHUNK = 2
} StringOwnership;

typedef struct Value Value;
typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct Chunk Chunk;
typedef struct FunctionObj {
    int arity;
    int upvalueCount;
    char* name;
    Chunk* chunk;
    bool ownsChunk;
} FunctionObj;

typedef struct Upvalue Upvalue;
typedef struct ClosureObj ClosureObj;

typedef struct Value {
    ValueType type;
    union {
        int intValue;
        float floatValue;
        const char* stringValue;
        int boolValue;
        struct {
            struct ASTNode* declaration;
            struct Env* closure;
        } functionValue;
        FunctionObj* functionObj;
        ClosureObj* closureObj;
        struct Value* returnValue; // Pointer to the wrapped return value
        NativeFn nativeFn;
        struct {
            int start;
            int end;
        } rangeValue;
        struct {
            struct Value* elements;
            int count;
            int capacity;
        } arrayValue;
    } data;
    StringOwnership stringOwnership;
    // Ownership policy:
    // - Borrowed values (isBorrowed = true) must not free heap data.
    // - Strings are owned when stringOwnership == STRING_OWNERSHIP_VALUE.
    // - Arrays are owned by the Value unless borrowed.
    // - Functions share closures via env_ref/env_unref.
    bool isBorrowed;
    int line;
    int column;
} Value;

struct Upvalue {
    Value* location;
    Value closed;
    Upvalue* next;
    Upvalue* nextAll;
};

struct ClosureObj {
    FunctionObj* function;
    Upvalue** upvalues;
    int upvalueCount;
    ClosureObj* next;
};

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

struct ASTNode;
struct Env;

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);

Value makeInt(int i);
Value makeFloat(float f);
Value makeString(const char* s);
Value makeStringOwned(const char* s);
Value makeBool(int b);
Value makeNull();
Value makeFunction(struct ASTNode* declaration, struct Env* closure);
Value makeFunctionObj(FunctionObj* fn);
Value makeClosure(ClosureObj* closure);
Value makeReturn(Value value);
Value makeBreak(void);
Value makeContinue(void);
Value makeNative(NativeFn fn);
Value makeRange(int start, int end);
Value makeArray(int count, Value* elements);
Value valueMakeOwned(Value v);
void freeValueContents(Value v);
void printValue(Value v);
int isTruthy(Value v);
char* valueToString(Value v);

#endif //ESPRESSO_VALUE_H
