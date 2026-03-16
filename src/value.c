// value.c
// Created by Derek Roper on 3/9/26.
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "value.h"
#include "env.h"

Value makeInt(int i) {
    Value t;
    t.type = VALUE_INT;
    t.data.intValue = i;
    t.stringOwnership = STRING_OWNERSHIP_NONE;
    t.line = 0;
    t.column = 0;
    return t;
}

Value makeString(const char* s) {
    Value t;
    t.type = VALUE_STRING;
    t.data.stringValue = s;
    t.stringOwnership = STRING_OWNERSHIP_NONE;
    t.line = 0;
    t.column = 0;
    return t;
}

Value makeStringOwned(const char* s) {
    Value t;
    t.type = VALUE_STRING;
    t.data.stringValue = strdup(s);
    t.stringOwnership = STRING_OWNERSHIP_VALUE;
    t.line = 0;
    t.column = 0;
    return t;
}

Value makeNull() {
    Value t;
    t.type = VALUE_NULL;
    t.stringOwnership = STRING_OWNERSHIP_NONE;
    t.line = 0;
    t.column = 0;
    return t;
}

Value makeFunction(struct ASTNode* declaration, struct Env* closure) {
    Value t;
    t.type = VALUE_FUNCTION;
    t.data.functionValue.declaration = declaration;
    t.data.functionValue.closure = closure;
    if (closure) env_ref(closure);
    t.stringOwnership = STRING_OWNERSHIP_NONE;
    t.line = 0;
    t.column = 0;
    return t;
}

Value makeReturn(Value value) {
    Value t;
    t.type = VALUE_RETURN;
    t.data.returnValue = malloc(sizeof(Value));
    *(t.data.returnValue) = value;
    t.stringOwnership = STRING_OWNERSHIP_NONE;
    t.line = 0;
    t.column = 0;
    return t;
}

void freeValueContents(Value v) {
    if (v.type == VALUE_STRING && v.stringOwnership == STRING_OWNERSHIP_VALUE) {
        free((void*)v.data.stringValue);
        return;
    }
    if (v.type == VALUE_FUNCTION) {
        env_unref(v.data.functionValue.closure);
    } else if (v.type == VALUE_RETURN) {
        freeValueContents(*(v.data.returnValue));
        free(v.data.returnValue);
    } else if (v.type == VALUE_ARRAY) {
        for (int i = 0; i < v.data.arrayValue.count; i++) {
            freeValueContents(v.data.arrayValue.elements[i]);
        }
        free(v.data.arrayValue.elements);
    }
}

Value makeBool(int b) {
    Value t;
    t.type = VALUE_BOOL;
    t.data.boolValue = b;
    t.stringOwnership = STRING_OWNERSHIP_NONE;
    t.line = 0;
    t.column = 0;
    return t;
}

Value makeFloat(float f) {
    Value t;
    t.type = VALUE_FLOAT;
    t.data.floatValue = f;
    t.stringOwnership = STRING_OWNERSHIP_NONE;
    t.line = 0;
    t.column = 0;
    return t;
}

void printValue(Value v) {
    switch (v.type) {
        case VALUE_INT:      printf("%d", v.data.intValue);                                    break;
        case VALUE_FLOAT:    printf("%g", v.data.floatValue);                                  break;
        case VALUE_STRING:   printf("%s", v.data.stringValue);                                 break;
        case VALUE_BOOL:     printf("%s", v.data.boolValue ? "true" : "false");                break;
        case VALUE_NULL:     printf("null");                                                   break;
        case VALUE_FUNCTION: printf("<fn>");                                                   break;
        case VALUE_NATIVE:   printf("<native fn>");                                            break;
        case VALUE_RANGE:    printf("%d..%d", v.data.rangeValue.start, v.data.rangeValue.end); break;
        case VALUE_ARRAY: {
            printf("[");
            for (int i = 0; i < v.data.arrayValue.count; i++) {
                printValue(v.data.arrayValue.elements[i]);
                if (i < v.data.arrayValue.count - 1) printf(", ");
            }
            printf("]");
            break;
        }
        case VALUE_RETURN:   printValue(*(v.data.returnValue));                                break;
        default:           printf("<unknown>");                                                break;
    }
}

int isTruthy(Value v) {
    switch (v.type) {
        case VALUE_INT:    return v.data.intValue != 0;
        case VALUE_FLOAT:  return v.data.floatValue != 0.0f;
        case VALUE_BOOL:   return v.data.boolValue != 0;
        case VALUE_STRING: return v.data.stringValue != NULL && v.data.stringValue[0] != '\0';
        case VALUE_NULL:   return 0;
        case VALUE_RETURN: return isTruthy(*(v.data.returnValue));
        default:           return 0;
    }
}

Value makeNative(NativeFn fn) {
    Value t;
    t.type = VALUE_NATIVE;
    t.data.nativeFn = fn;
    t.stringOwnership = STRING_OWNERSHIP_NONE;
    t.line = 0;
    t.column = 0;
    return t;
}

Value makeRange(int start, int end) {
    Value t;
    t.type = VALUE_RANGE;
    t.data.rangeValue.start = start;
    t.data.rangeValue.end = end;
    t.stringOwnership = STRING_OWNERSHIP_NONE;
    t.line = 0;
    t.column = 0;
    return t;
}

char* valueToString(Value v) {
    char buf[128];
    switch (v.type) {
        case VALUE_INT:      snprintf(buf, sizeof(buf), "%d", v.data.intValue);    break;
        case VALUE_FLOAT:    snprintf(buf, sizeof(buf), "%g", v.data.floatValue);  break;
        case VALUE_STRING:   return strdup(v.data.stringValue);
        case VALUE_BOOL:     return strdup(v.data.boolValue ? "true" : "false");
        case VALUE_NULL:     return strdup("null");
        case VALUE_FUNCTION: return strdup("<fn>");
        case VALUE_NATIVE:   return strdup("<native fn>");
        case VALUE_RANGE:    snprintf(buf, sizeof(buf), "%d..%d", v.data.rangeValue.start, v.data.rangeValue.end); break;
        case VALUE_ARRAY: {
            // Primitive string builder for arrays
            int cap = 128;
            char* res = malloc(cap);
            strcpy(res, "[");
            for (int i = 0; i < v.data.arrayValue.count; i++) {
                char* s = valueToString(v.data.arrayValue.elements[i]);
                if (strlen(res) + strlen(s) + 5 >= (size_t)cap) {
                    cap *= 2;
                    res = realloc(res, cap);
                }
                strcat(res, s);
                if (i < v.data.arrayValue.count - 1) strcat(res, ", ");
                free(s);
            }
            strcat(res, "]");
            return res;
        }
        case VALUE_RETURN:   return valueToString(*(v.data.returnValue));
        default:           return strdup("<unknown>");
    }
    return strdup(buf);
}

Value makeArray(int count, Value* elements) {
    Value v;
    v.type = VALUE_ARRAY;
    v.data.arrayValue.count = count;
    v.data.arrayValue.capacity = count;
    v.data.arrayValue.elements = elements;
    v.stringOwnership = STRING_OWNERSHIP_NONE;
    v.line = 0;
    v.column = 0;
    return v;
}

void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = oldCapacity < 8 ? 8 : oldCapacity * 2;
        array->values = realloc(array->values, sizeof(Value) * array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray* array) {
    for (int i = 0; i < array->count; i++) {
        Value v = array->values[i];
        if (v.type == VALUE_STRING && v.stringOwnership == STRING_OWNERSHIP_CHUNK) {
            free((void*)v.data.stringValue);
        } else {
            freeValueContents(v);
        }
    }
    free(array->values);
    initValueArray(array);
}
