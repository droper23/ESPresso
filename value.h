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
    VALUE_NIL
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        int intValue;
        float floatValue;
        const char* stringValue;
        int boolValue;
    } data;
} Value;

Value makeInt(int i);
Value makeFloat(float f);
Value makeString(const char* s);
Value makeBool(int b);
Value makeNil();

#endif //ESPRESSO_VALUE_H