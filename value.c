// value.c
// Created by Derek Roper on 3/9/26.
//
#include <string.h>
#include<stdlib.h>
#include "value.h"
#include "env.h"

Value makeInt(int i) {
    Value t;
    t.type = VALUE_INT;
    t.data.intValue = i;
    return t;
}

Value makeString(const char* s) {
    Value t;
    t.type = VALUE_STRING;
    t.data.stringValue = s;
    return t;
}

Value makeNil() {
    Value t;
    t.type = VALUE_NIL;
    return t;
}

Value makeFunction(struct ASTNode* declaration, struct Env* closure) {
    Value t;
    t.type = VALUE_FUNCTION;
    t.data.functionValue.declaration = declaration;
    t.data.functionValue.closure = closure;
    if (closure) env_ref(closure);
    return t;
}

Value makeReturn(Value value) {
    Value t;
    t.type = VALUE_RETURN;
    t.data.returnValue = malloc(sizeof(Value));
    *(t.data.returnValue) = value;
    return t;
}

void freeValueContents(Value v) {
    if (v.type == VALUE_FUNCTION) {
        env_unref(v.data.functionValue.closure);
    } else if (v.type == VALUE_RETURN) {
        freeValueContents(*(v.data.returnValue));
        free(v.data.returnValue);
    }
}

Value makeBool(int b) {
    Value t;
    t.type = VALUE_BOOL;
    t.data.boolValue = b;
    return t;
}

Value makeFloat(float f) {
    Value t;
    t.type = VALUE_FLOAT;
    t.data.floatValue = f;
    return t;
}

