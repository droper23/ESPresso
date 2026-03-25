// vm.c
// Created by Derek Roper on 3/16/26.
//

#include "vm.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK_MAX 256
#define FRAMES_MAX 64

typedef struct {
    ClosureObj* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    Value stack[STACK_MAX];
    Value* stackTop;
    Env* globals;
    int trace;
    Upvalue* openUpvalues;
    Upvalue* allUpvalues;
    ClosureObj* allClosures;
    FunctionObj* scriptFunction;
} VM;

static VM vm;
static int traceEnabled = 0;
static const char* vmSource = NULL;
static const char* vmSourcePath = NULL;

void vmSetTrace(int enabled) {
    traceEnabled = enabled ? 1 : 0;
}

void vmSetSource(const char* source, const char* path) {
    vmSource = source;
    vmSourcePath = path;
}

static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);

    if (vm.frameCount == 0) return;
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    Chunk* chunk = frame->closure->function->chunk;
    size_t instruction = frame->ip - chunk->code - 1;
    int line = chunk->lines[instruction];
    int column = chunk->columns[instruction];
    if (column > 0) {
        fprintf(stderr, "[line %d, col %d] in script\n", line, column);
    } else {
        fprintf(stderr, "[line %d] in script\n", line);
    }

    if (vmSource) {
        const char* p = vmSource;
        const char* lineStart = vmSource;
        int currentLine = 1;
        while (*p && currentLine < line) {
            if (*p == '\n') {
                currentLine++;
                lineStart = p + 1;
            }
            p++;
        }
        const char* lineEnd = lineStart;
        while (*lineEnd && *lineEnd != '\n') lineEnd++;
        fprintf(stderr, "  %.*s\n", (int)(lineEnd - lineStart), lineStart);
        if (column > 0) {
            fprintf(stderr, "  ");
            for (int i = 1; i < column; i++) fputc(' ', stderr);
            fputc('^', stderr);
            fputc('\n', stderr);
        }
    }

    fprintf(stderr, "Stack trace:\n");
    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* f = &vm.frames[i];
        Chunk* c = f->closure->function->chunk;
        size_t ipIndex = (f->ip > c->code) ? (size_t)(f->ip - c->code - 1) : 0;
        int l = c->lines[ipIndex];
        int col = c->columns[ipIndex];
        const char* name = f->closure->function->name ? f->closure->function->name : "script";
        if (vmSourcePath) {
            if (col > 0) {
                fprintf(stderr, "  at %s (%s:%d:%d)\n", name, vmSourcePath, l, col);
            } else {
                fprintf(stderr, "  at %s (%s:%d)\n", name, vmSourcePath, l);
            }
        } else {
            if (col > 0) {
                fprintf(stderr, "  at %s (line %d, col %d)\n", name, l, col);
            } else {
                fprintf(stderr, "  at %s (line %d)\n", name, l);
            }
        }
    }
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

static Upvalue* newUpvalue(Value* slot) {
    Upvalue* upvalue = malloc(sizeof(Upvalue));
    upvalue->location = slot;
    upvalue->closed = makeNull();
    upvalue->next = NULL;
    upvalue->nextAll = vm.allUpvalues;
    vm.allUpvalues = upvalue;
    return upvalue;
}

static Upvalue* captureUpvalue(Value* local) {
    Upvalue* prev = NULL;
    Upvalue* curr = vm.openUpvalues;
    while (curr != NULL && curr->location > local) {
        prev = curr;
        curr = curr->next;
    }

    if (curr != NULL && curr->location == local) {
        return curr;
    }

    Upvalue* created = newUpvalue(local);
    created->next = curr;
    if (prev == NULL) {
        vm.openUpvalues = created;
    } else {
        prev->next = created;
    }
    return created;
}

static void closeUpvalues(Value* last) {
    while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        Upvalue* upvalue = vm.openUpvalues;
        Value* location = upvalue->location;
        upvalue->closed = *location;
        upvalue->location = &upvalue->closed;
        *location = makeNull();
        vm.openUpvalues = upvalue->next;
    }
}

static ClosureObj* newClosure(FunctionObj* function) {
    ClosureObj* closure = malloc(sizeof(ClosureObj));
    closure->function = function;
    closure->upvalueCount = function->upvalueCount;
    closure->upvalues = NULL;
    if (closure->upvalueCount > 0) {
        closure->upvalues = malloc(sizeof(Upvalue*) * closure->upvalueCount);
        for (int i = 0; i < closure->upvalueCount; i++) {
            closure->upvalues[i] = NULL;
        }
    }
    closure->next = vm.allClosures;
    vm.allClosures = closure;
    return closure;
}

static int callClosure(ClosureObj* closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError("Expected %d arguments but got %d.", closure->function->arity, argCount);
        return 0;
    }
    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return 0;
    }
    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk->code;
    frame->slots = vm.stackTop - argCount - 1;
    return 1;
}

static void freeUpvalue(Upvalue* upvalue) {
    if (upvalue->location == &upvalue->closed) {
        freeValueContents(upvalue->closed);
    }
    free(upvalue);
}

static void freeClosure(ClosureObj* closure) {
    free(closure->upvalues);
    free(closure);
}

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
        case OP_RANGE:         return "OP_RANGE";
        case OP_RANGE_START:   return "OP_RANGE_START";
        case OP_RANGE_END:     return "OP_RANGE_END";
        case OP_CLOSURE:       return "OP_CLOSURE";
        case OP_GET_UPVALUE:   return "OP_GET_UPVALUE";
        case OP_SET_UPVALUE:   return "OP_SET_UPVALUE";
        case OP_CLOSE_UPVALUE: return "OP_CLOSE_UPVALUE";
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
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->closure->function->chunk->constants.values[READ_BYTE()])
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

    for (;;) {
        uint8_t instruction = READ_BYTE();
        if (vm.trace) {
            int offset = (int)(frame->ip - frame->closure->function->chunk->code - 1);
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
            case OP_POP: {
                Value v = pop();
                freeValueContents(v);
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                Value v = frame->slots[slot];
                v.isBorrowed = true;
                push(v);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                Value value = peek(0);
                freeValueContents(frame->slots[slot]);
                frame->slots[slot] = valueMakeOwned(value);
                break;
            }
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
                if (!env_set(vm.globals, name, value)) {
                    runtimeError("Undefined global '%s'.", name);
                }
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
                freeValueContents(v);
                break;
            }
            case OP_CALL: {
                uint8_t argCount = READ_BYTE();
                Value callee = peek(argCount);
                if (callee.type == VALUE_NATIVE) {
                    Value* args = vm.stackTop - argCount;
                    Value result = callee.data.nativeFn(argCount, args);
                    for (int i = 0; i < argCount; i++) {
                        freeValueContents(args[i]);
                    }
                    freeValueContents(callee);
                    vm.stackTop -= (argCount + 1);
                    push(result);
                    break;
                }
                if (callee.type != VALUE_CLOSURE) {
                    runtimeError("Can only call functions and native functions in VM.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (!callClosure(callee.data.closureObj, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_CLOSURE: {
                Value constant = READ_CONSTANT();
                if (constant.type != VALUE_FUNCTION_OBJ) {
                    runtimeError("Expected function object for closure.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ClosureObj* closure = newClosure(constant.data.functionObj);
                for (int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                push(makeClosure(closure));
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                Value v = *(frame->closure->upvalues[slot]->location);
                v.isBorrowed = true;
                push(v);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                Value value = peek(0);
                Upvalue* upvalue = frame->closure->upvalues[slot];
                freeValueContents(*(upvalue->location));
                *(upvalue->location) = valueMakeOwned(value);
                break;
            }
            case OP_CLOSE_UPVALUE: {
                closeUpvalues(vm.stackTop - 1);
                Value v = pop();
                (void)v;
                break;
            }
            case OP_ARRAY: {
                uint8_t count = READ_BYTE();
                Value* elements = NULL;
                if (count > 0) {
                    elements = malloc(sizeof(Value) * count);
                    for (int i = (int)count - 1; i >= 0; i--) {
                        Value v = pop();
                        elements[i] = valueMakeOwned(v);
                    }
                }
                push(makeArray(count, elements));
                break;
            }
            case OP_RANGE: {
                Value endVal = pop();
                Value startVal = pop();
                if (startVal.type != VALUE_INT || endVal.type != VALUE_INT) {
                    runtimeError("Range bounds must be integers.");
                    freeValueContents(startVal);
                    freeValueContents(endVal);
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value range = makeRange(startVal.data.intValue, endVal.data.intValue);
                freeValueContents(startVal);
                freeValueContents(endVal);
                push(range);
                break;
            }
            case OP_RANGE_START: {
                Value range = peek(0);
                if (range.type != VALUE_RANGE) {
                    runtimeError("Expected range value.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(makeInt(range.data.rangeValue.start));
                break;
            }
            case OP_RANGE_END: {
                Value range = peek(0);
                if (range.type != VALUE_RANGE) {
                    range = peek(1);
                }
                if (range.type != VALUE_RANGE) {
                    runtimeError("Expected range value.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(makeInt(range.data.rangeValue.end));
                break;
            }
            case OP_GET_INDEX: {
                Value index = pop();
                Value array = pop();
                if (array.type != VALUE_ARRAY) {
                    runtimeError("Indexing requires an array.");
                    freeValueContents(index);
                    freeValueContents(array);
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (index.type != VALUE_INT) {
                    runtimeError("Array index must be an integer.");
                    freeValueContents(index);
                    freeValueContents(array);
                    return INTERPRET_RUNTIME_ERROR;
                }
                int i = index.data.intValue;
                if (i < 0 || i >= array.data.arrayValue.count) {
                    runtimeError("Array index out of bounds.");
                    freeValueContents(index);
                    freeValueContents(array);
                    return INTERPRET_RUNTIME_ERROR;
                }
                Value elem = array.data.arrayValue.elements[i];
                Value result = array.isBorrowed ? elem : valueMakeOwned(elem);
                if (array.isBorrowed) {
                    result.isBorrowed = true;
                }
                freeValueContents(index);
                freeValueContents(array);
                push(result);
                break;
            }
            case OP_SET_INDEX: {
                Value index = pop();
                Value array = pop();
                Value value = pop();
                if (array.type != VALUE_ARRAY) {
                    runtimeError("Indexing requires an array.");
                    freeValueContents(value);
                    freeValueContents(index);
                    freeValueContents(array);
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (index.type != VALUE_INT) {
                    runtimeError("Array index must be an integer.");
                    freeValueContents(value);
                    freeValueContents(index);
                    freeValueContents(array);
                    return INTERPRET_RUNTIME_ERROR;
                }
                int i = index.data.intValue;
                if (i < 0 || i >= array.data.arrayValue.count) {
                    runtimeError("Array index out of bounds.");
                    freeValueContents(value);
                    freeValueContents(index);
                    freeValueContents(array);
                    return INTERPRET_RUNTIME_ERROR;
                }
                freeValueContents(array.data.arrayValue.elements[i]);
                array.data.arrayValue.elements[i] = valueMakeOwned(value);
                if (!array.isBorrowed) {
                    freeValueContents(array);
                }
                freeValueContents(index);
                Value result = valueMakeOwned(value);
                freeValueContents(value);
                push(result);
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (!isTruthy(peek(0))) frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_RETURN: {
                Value result = pop();
                closeUpvalues(frame->slots);
                while (vm.stackTop > frame->slots) {
                    Value v = pop();
                    freeValueContents(v);
                }
                vm.frameCount--;
                if (vm.frameCount == 0) {
                    freeValueContents(result);
                    return INTERPRET_OK;
                }
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            default:
                runtimeError("Unknown opcode %d.", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
}

InterpretResult interpret(Chunk* chunk, Env* globals) {
    vm.globals = globals;
    vm.trace = traceEnabled;
    resetStack();
    vm.allUpvalues = NULL;
    vm.allClosures = NULL;

    vm.scriptFunction = malloc(sizeof(FunctionObj));
    vm.scriptFunction->arity = 0;
    vm.scriptFunction->upvalueCount = 0;
    vm.scriptFunction->name = strdup("script");
    vm.scriptFunction->chunk = chunk;
    vm.scriptFunction->ownsChunk = false;

    ClosureObj* scriptClosure = newClosure(vm.scriptFunction);
    push(makeClosure(scriptClosure));
    callClosure(scriptClosure, 0);

    return run();
}

void vmFree(void) {
    closeUpvalues(vm.stack);

    Upvalue* upvalue = vm.allUpvalues;
    while (upvalue) {
        Upvalue* next = upvalue->nextAll;
        freeUpvalue(upvalue);
        upvalue = next;
    }
    vm.allUpvalues = NULL;

    ClosureObj* closure = vm.allClosures;
    while (closure) {
        ClosureObj* next = closure->next;
        freeClosure(closure);
        closure = next;
    }
    vm.allClosures = NULL;

    if (vm.scriptFunction) {
        if (vm.scriptFunction->name) free(vm.scriptFunction->name);
        free(vm.scriptFunction);
        vm.scriptFunction = NULL;
    }
}
