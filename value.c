// value.c
// Created by Derek Roper on 3/9/26.
//
#include <string.h>
#include<stdlib.h>
#include "value.h"

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

