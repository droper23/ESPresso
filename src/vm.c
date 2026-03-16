// vm.c
// Created by Derek Roper on 3/16/26.
//

#include "vm.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK_MAX 256

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    Value stack[STACK_MAX];
    Value* stackTop;
    Env* globals;
    int trace;
} VM;

static VM vm;
static int traceEnabled = 0;

void vmSetTrace(int enabled) {
    traceEnabled = enabled ? 1 : 0;
}

static void resetStack() {
    vm.stackTop = vm.stack;
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
}

static void push(Value value) {
    if ((vm.stackTop - vm.stack) >= STACK_MAX) {
        runtimeError("Stack overflow.");
        return;
    }
    *vm.stackTop = value;
    vm.stackTop++;
}

static Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))

static const char* opcodeName(uint8_t opcode) {
    switch (opcode) {
        case OP_CONSTANT:      return "OP_CONSTANT";
        case OP_NULL:          return "OP_NULL";
        case OP_TRUE:          return "OP_TRUE";
        case OP_FALSE:         return "OP_FALSE";
        case OP_POP:           return "OP_POP";
        case OP_GET_LOCAL:     return "OP_GET_LOCAL";
        case OP_SET_LOCAL:     return "OP_SET_LOCAL";
        case OP_GET_GLOBAL:    return "OP_GET_GLOBAL";
        case OP_DEFINE_GLOBAL: return "OP_DEFINE_GLOBAL";
        case OP_SET_GLOBAL:    return "OP_SET_GLOBAL";
        case OP_GET_INDEX:     return "OP_GET_INDEX";
        case OP_SET_INDEX:     return "OP_SET_INDEX";
        case OP_EQUAL:         return "OP_EQUAL";
        case OP_GREATER:       return "OP_GREATER";
        case OP_LESS:          return "OP_LESS";
        case OP_ADD:           return "OP_ADD";
        case OP_SUBTRACT:      return "OP_SUBTRACT";
        case OP_MULTIPLY:      return "OP_MULTIPLY";
        case OP_DIVIDE:        return "OP_DIVIDE";
        case OP_NOT:           return "OP_NOT";
        case OP_NEGATE:        return "OP_NEGATE";
        case OP_PRINT:         return "OP_PRINT";
        case OP_JUMP:          return "OP_JUMP";
        case OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
        case OP_LOOP:          return "OP_LOOP";
        case OP_CALL:          return "OP_CALL";
        case OP_ARRAY:         return "OP_ARRAY";
        case OP_RETURN:        return "OP_RETURN";
        default:               return "OP_UNKNOWN";
    }
}

static int valuesEqual(Value a, Value b) {
    if (a.type != b.type) {
        if ((a.type == VALUE_INT && b.type == VALUE_FLOAT) ||
            (a.type == VALUE_FLOAT && b.type == VALUE_INT)) {
            float af = (a.type == VALUE_FLOAT) ? a.data.floatValue : (float)a.data.intValue;
            float bf = (b.type == VALUE_FLOAT) ? b.data.floatValue : (float)b.data.intValue;
            return af == bf;
        }
        return 0;
    }

    switch (a.type) {
        case VALUE_NULL:   return 1;
        case VALUE_BOOL:   return a.data.boolValue == b.data.boolValue;
        case VALUE_INT:    return a.data.intValue == b.data.intValue;
        case VALUE_FLOAT:  return a.data.floatValue == b.data.floatValue;
        case VALUE_STRING: return strcmp(a.data.stringValue, b.data.stringValue) == 0;
        default:           return 0;
    }
}

static InterpretResult run() {
    for (;;) {
        uint8_t instruction = READ_BYTE();
        if (vm.trace) {
            int offset = (int)(vm.ip - vm.chunk->code - 1);
            printf("%04d %-16s\n", offset, opcodeName(instruction));
        }
        switch (instruction) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NULL:   push(makeNull()); break;
            case OP_TRUE:   push(makeBool(1)); break;
            case OP_FALSE:  push(makeBool(0)); break;
            case OP_POP:    pop(); break;
            case OP_GET_GLOBAL: {
                Value nameVal = READ_CONSTANT();
                const char* name = nameVal.data.stringValue;
                Value value = env_get(vm.globals, name);
                push(value);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                Value nameVal = READ_CONSTANT();
                const char* name = nameVal.data.stringValue;
                Value value = pop();
                env_define(vm.globals, name, value);
                break;
            }
            case OP_SET_GLOBAL: {
                Value nameVal = READ_CONSTANT();
                const char* name = nameVal.data.stringValue;
                Value value = peek(0);
                env_set(vm.globals, name, value);
                break;
            }
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(makeBool(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: {
                Value b = pop();
                Value a = pop();
                float af = (a.type == VALUE_FLOAT) ? a.data.floatValue : (float)a.data.intValue;
                float bf = (b.type == VALUE_FLOAT) ? b.data.floatValue : (float)b.data.intValue;
                push(makeBool(af > bf));
                break;
            }
            case OP_LESS: {
                Value b = pop();
                Value a = pop();
                float af = (a.type == VALUE_FLOAT) ? a.data.floatValue : (float)a.data.intValue;
                float bf = (b.type == VALUE_FLOAT) ? b.data.floatValue : (float)b.data.intValue;
                push(makeBool(af < bf));
                break;
            }
            case OP_ADD: {
                Value b = pop();
                Value a = pop();
                if (a.type == VALUE_STRING || b.type == VALUE_STRING) {
                    char* aStr = valueToString(a);
                    char* bStr = valueToString(b);
                    size_t len = strlen(aStr) + strlen(bStr) + 1;
                    char* buf = malloc(len);
                    strcpy(buf, aStr);
                    strcat(buf, bStr);
                    Value result = makeStringOwned(buf);
                    free(buf);
                    free(aStr);
                    free(bStr);
                    push(result);
                } else {
                    if (a.type == VALUE_FLOAT || b.type == VALUE_FLOAT) {
                        float af = (a.type == VALUE_FLOAT) ? a.data.floatValue : (float)a.data.intValue;
                        float bf = (b.type == VALUE_FLOAT) ? b.data.floatValue : (float)b.data.intValue;
                        push(makeFloat(af + bf));
                    } else {
                        push(makeInt(a.data.intValue + b.data.intValue));
                    }
                }
                break;
            }
            case OP_SUBTRACT: {
                Value b = pop();
                Value a = pop();
                if (a.type == VALUE_FLOAT || b.type == VALUE_FLOAT) {
                    float af = (a.type == VALUE_FLOAT) ? a.data.floatValue : (float)a.data.intValue;
                    float bf = (b.type == VALUE_FLOAT) ? b.data.floatValue : (float)b.data.intValue;
                    push(makeFloat(af - bf));
                } else {
                    push(makeInt(a.data.intValue - b.data.intValue));
                }
                break;
            }
            case OP_MULTIPLY: {
                Value b = pop();
                Value a = pop();
                if (a.type == VALUE_FLOAT || b.type == VALUE_FLOAT) {
                    float af = (a.type == VALUE_FLOAT) ? a.data.floatValue : (float)a.data.intValue;
                    float bf = (b.type == VALUE_FLOAT) ? b.data.floatValue : (float)b.data.intValue;
                    push(makeFloat(af * bf));
                } else {
                    push(makeInt(a.data.intValue * b.data.intValue));
                }
                break;
            }
            case OP_DIVIDE: {
                Value b = pop();
                Value a = pop();
                float af = (a.type == VALUE_FLOAT) ? a.data.floatValue : (float)a.data.intValue;
                float bf = (b.type == VALUE_FLOAT) ? b.data.floatValue : (float)b.data.intValue;
                if (bf == 0.0f) {
                    runtimeError("Division by zero.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (a.type == VALUE_FLOAT || b.type == VALUE_FLOAT) {
                    push(makeFloat(af / bf));
                } else {
                    push(makeInt((int)(af / bf)));
                }
                break;
            }
            case OP_NOT: {
                Value v = pop();
                push(makeBool(!isTruthy(v)));
                break;
            }
            case OP_NEGATE: {
                Value v = pop();
                if (v.type == VALUE_FLOAT) push(makeFloat(-v.data.floatValue));
                else push(makeInt(-v.data.intValue));
                break;
            }
            case OP_PRINT: {
                Value v = pop();
                printValue(v);
                printf("\n");
                break;
            }
            case OP_CALL: {
                uint8_t argCount = READ_BYTE();
                Value callee = peek(argCount);
                if (callee.type != VALUE_NATIVE) {
                    runtimeError("Can only call native functions in VM for now.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value* args = vm.stackTop - argCount;
                Value result = callee.data.nativeFn(argCount, args);
                vm.stackTop -= (argCount + 1);
                push(result);
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                vm.ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (!isTruthy(peek(0))) vm.ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                vm.ip -= offset;
                break;
            }
            case OP_RETURN:
                return INTERPRET_OK;
            default:
                runtimeError("Unknown opcode %d.", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
    }
}

InterpretResult interpret(Chunk* chunk, Env* globals) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    vm.globals = globals;
    vm.trace = traceEnabled;
    resetStack();
    return run();
}
