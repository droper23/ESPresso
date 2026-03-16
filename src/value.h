// value.h
// Created by Derek Roper on 3/9/26.
//

#ifndef ESPRESSO_VALUE_H
#define ESPRESSO_VALUE_H

typedef enum {
    VALUE_INT,
    VALUE_FLOAT,
    VALUE_STRING,
    VALUE_BOOL,
    VALUE_NULL,
    VALUE_FUNCTION,
    VALUE_RETURN,
    VALUE_NATIVE,
    VALUE_RANGE,
    VALUE_ARRAY
} ValueType;

typedef struct Value Value;
typedef Value (*NativeFn)(int argCount, Value* args);

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
    int line;
    int column;
} Value;

struct ASTNode;
struct Env;

Value makeInt(int i);
Value makeFloat(float f);
Value makeString(const char* s);
Value makeBool(int b);
Value makeNull();
Value makeFunction(struct ASTNode* declaration, struct Env* closure);
Value makeReturn(Value value);
Value makeNative(NativeFn fn);
Value makeRange(int start, int end);
Value makeArray(int count, Value* elements);
void freeValueContents(Value v);
void printValue(Value v);
int isTruthy(Value v);
char* valueToString(Value v);

#endif //ESPRESSO_VALUE_H