// compiler.c
// Created by Derek Roper on 3/16/26.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "compiler.h"
#include "parser.h"
#include "chunk.h"

typedef struct {
    Token name;
    int depth;
    bool isCaptured;
} Local;

typedef struct {
    uint8_t index;
    bool isLocal;
} UpvalueInfo;

typedef struct Loop {
    struct Loop* enclosing;
    int start;
    int scopeDepth;
    int breakCount;
    int breakJumps[256];
    int continueCount;
    int continueJumps[256];
    int continueTarget;
} Loop;

typedef struct Compiler {
    struct Compiler* enclosing;
    Chunk* chunk;

    Local locals[256];
    int localCount;
    int scopeDepth;
    UpvalueInfo upvalues[256];
    int upvalueCount;
    Loop* currentLoop;
    bool hadError;
} Compiler;

static void initCompiler(Compiler* compiler, Chunk* chunk, struct Compiler* enclosing) {
    compiler->enclosing = enclosing;
    compiler->chunk = chunk;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->upvalueCount = 0;
    compiler->currentLoop = NULL;
    compiler->hadError = false;

    Local* local = &compiler->locals[compiler->localCount++];
    local->depth = 0;
    local->name.lexeme = "";
    local->isCaptured = false;
}

static void compilerErrorAt(Compiler* compiler, int line, int column, const char* fmt, ...) {
    va_list args;
    compiler->hadError = true;
    va_start(args, fmt);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, fmt, args);
    if (line > 0) {
        fprintf(stderr, " at [Line %d, Col %d]", line, column);
    }
    fprintf(stderr, "\n");
    va_end(args);
}

static Chunk* currentChunk(Compiler* compiler) {
    return compiler->chunk;
}

static void emitByte(Compiler* compiler, uint8_t byte, int line, int column) {
    writeChunk(currentChunk(compiler), byte, line, column);
}

static void emitBytes(Compiler* compiler, uint8_t byte1, uint8_t byte2, int line, int column) {
    emitByte(compiler, byte1, line, column);
    emitByte(compiler, byte2, line, column);
}

static uint8_t makeConstant(Compiler* compiler, Value value) {
    int constant = addConstant(currentChunk(compiler), value);
    if (constant > UINT8_MAX) {
        compilerErrorAt(compiler, 0, 0, "too many constants in one chunk");
        return 0;
    }
    return (uint8_t)constant;
}

static void emitConstant(Compiler* compiler, Value value, int line, int column) {
    emitBytes(compiler, OP_CONSTANT, makeConstant(compiler, value), line, column);
}

static int emitJump(Compiler* compiler, uint8_t instruction, int line, int column) {
    emitByte(compiler, instruction, line, column);
    emitByte(compiler, 0xff, line, column);
    emitByte(compiler, 0xff, line, column);
    return currentChunk(compiler)->count - 2;
}

static void patchJump(Compiler* compiler, int offset) {
    int jump = currentChunk(compiler)->count - offset - 2;
    if (jump > UINT16_MAX) {
        compilerErrorAt(compiler, 0, 0, "too much code to jump over");
    }
    currentChunk(compiler)->code[offset] = (jump >> 8) & 0xff;
    currentChunk(compiler)->code[offset + 1] = jump & 0xff;
}

static void patchJumpTo(Compiler* compiler, int offset, int target) {
    int jump = target - offset - 2;
    if (jump < 0 || jump > UINT16_MAX) {
        compilerErrorAt(compiler, 0, 0, "invalid jump patch");
    }
    currentChunk(compiler)->code[offset] = (jump >> 8) & 0xff;
    currentChunk(compiler)->code[offset + 1] = jump & 0xff;
}

static void emitLoop(Compiler* compiler, int loopStart, int line, int column) {
    emitByte(compiler, OP_LOOP, line, column);
    int offset = currentChunk(compiler)->count - loopStart + 2;
    if (offset > UINT16_MAX) {
        compilerErrorAt(compiler, 0, 0, "loop body too large");
    }
    emitByte(compiler, (offset >> 8) & 0xff, line, column);
    emitByte(compiler, offset & 0xff, line, column);
}

static void emitLoopCleanup(Compiler* compiler, int scopeDepth, int line, int column) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        if (compiler->locals[i].depth <= scopeDepth) break;
        if (compiler->locals[i].isCaptured) {
            emitByte(compiler, OP_CLOSE_UPVALUE, line, column);
        } else {
            emitByte(compiler, OP_POP, line, column);
        }
    }
}

static void beginScope(Compiler* compiler) {
    compiler->scopeDepth++;
}

static void endScope(Compiler* compiler, int line, int column) {
    compiler->scopeDepth--;
    while (compiler->localCount > 0 &&
            compiler->locals[compiler->localCount - 1].depth >
            compiler->scopeDepth) {
        if (compiler->locals[compiler->localCount - 1].isCaptured) {
            emitByte(compiler, OP_CLOSE_UPVALUE, line, column);
        } else {
            emitByte(compiler, OP_POP, line, column);
        }
        compiler->localCount--;
    }
}

static void addLocal(Compiler* compiler, Token name) {
    if (compiler->localCount == 256) {
        compilerErrorAt(compiler, 0, 0, "too many local variables in function");
        return;
    }
    Local* local = &compiler->locals[compiler->localCount++];
    local->name = name;
    local->depth = -1; // Mark as uninitialized
    local->isCaptured = false;
}

static int resolveLocal(Compiler* compiler, const char* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (local->name.lexeme && strcmp(local->name.lexeme, name) == 0) {
            if (local->depth == -1) {
                compilerErrorAt(compiler, 0, 0,
                                "cannot read local variable '%s' in its own initializer",
                                local->name.lexeme ? local->name.lexeme : "");
            }
            return i;
        }
    }
    return -1;
}

static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->upvalueCount;
    for (int i = 0; i < upvalueCount; i++) {
        UpvalueInfo* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }
    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    compiler->upvalueCount++;
    return upvalueCount;
}

static int resolveUpvalue(Compiler* compiler, const char* name) {
    if (compiler->enclosing == NULL) return -1;

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t)upvalue, false);
    }

    return -1;
}
static void markInitialized(Compiler* compiler) {
    if (compiler->scopeDepth == 0) return;
    compiler->locals[compiler->localCount - 1].depth = compiler->scopeDepth;
}

static void compileNode(Compiler* compiler, ASTNode* node);
static void emitDiscardIfNeeded(Compiler* compiler, ASTNode* stmt);
static int tempVarId = 0;

static uint8_t argumentList(Compiler* compiler, ASTNode* argList) {
    uint8_t argCount = 0;
    ASTNode* current = argList;
    while (current) {
        compileNode(compiler, current);
        argCount++;
        current = current->next;
    }
    return argCount;
}

static void emitDiscardIfNeeded(Compiler* compiler, ASTNode* stmt) {
    switch (stmt->type) {
        case NODE_ASSIGN:
        case NODE_AUGMENTED_ASSIGN:
        case NODE_BINARY_OP:
        case NODE_NUMBER:
        case NODE_STRING:
        case NODE_IDENTIFIER:
        case NODE_FUNCTION:
        case NODE_ARRAY:
        case NODE_ARRAY_INDEX:
        case NODE_RANGE:
            emitByte(compiler, OP_POP, stmt->line, stmt->column);
            break;
        default:
            break;
    }
}

static void compileFunction(Compiler* compiler, ASTNode* node) {
    Compiler fnCompiler;
    Chunk* fnChunk = malloc(sizeof(Chunk));
    initChunk(fnChunk);
    initCompiler(&fnCompiler, fnChunk, compiler);
    fnCompiler.scopeDepth = 1;

    int arity = 0;
    ASTNode* param = node->left;
    while (param) {
        Token t = {0};
        t.lexeme = param->name;
        t.ownsLexeme = false;
        addLocal(&fnCompiler, t);
        markInitialized(&fnCompiler);
        arity++;
        param = param->next;
    }

    compileNode(&fnCompiler, node->body);
    emitByte(&fnCompiler, OP_NULL, node->line, node->column);
    emitByte(&fnCompiler, OP_RETURN, node->line, node->column);

    if (fnCompiler.hadError) {
        compiler->hadError = true;
        freeChunk(fnChunk);
        free(fnChunk);
        return;
    }

    FunctionObj* fn = malloc(sizeof(FunctionObj));
    fn->arity = arity;
    fn->upvalueCount = fnCompiler.upvalueCount;
    fn->name = node->name ? strdup(node->name) : NULL;
    fn->chunk = fnChunk;
    fn->ownsChunk = true;

    uint8_t constant = makeConstant(compiler, makeFunctionObj(fn));
    emitBytes(compiler, OP_CLOSURE, constant, node->line, node->column);
    for (int i = 0; i < fn->upvalueCount; i++) {
        emitByte(compiler, fnCompiler.upvalues[i].isLocal ? 1 : 0, node->line, node->column);
        emitByte(compiler, fnCompiler.upvalues[i].index, node->line, node->column);
    }
}

static void compileBinaryOp(Compiler* compiler, ASTNode* node) {
    if (strcmp(node->name, "and") == 0) {
        compileNode(compiler, node->left);
        int endJump = emitJump(compiler, OP_JUMP_IF_FALSE, node->line, node->column);
        emitByte(compiler, OP_POP, node->line, node->column);
        compileNode(compiler, node->right);
        patchJump(compiler, endJump);
        return;
    } else if (strcmp(node->name, "or") == 0) {
        compileNode(compiler, node->left);
        int elseJump = emitJump(compiler, OP_JUMP_IF_FALSE, node->line, node->column);
        int endJump = emitJump(compiler, OP_JUMP, node->line, node->column);
        patchJump(compiler, elseJump);
        emitByte(compiler, OP_POP, node->line, node->column);
        compileNode(compiler, node->right);
        patchJump(compiler, endJump);
        return;
    }

    compileNode(compiler, node->left);
    compileNode(compiler, node->right);

    if (strcmp(node->name, "+") == 0)         emitByte(compiler, OP_ADD, node->line, node->column);
    else if (strcmp(node->name, "-") == 0)    emitByte(compiler, OP_SUBTRACT, node->line, node->column);
    else if (strcmp(node->name, "*") == 0)    emitByte(compiler, OP_MULTIPLY, node->line, node->column);
    else if (strcmp(node->name, "/") == 0)    emitByte(compiler, OP_DIVIDE, node->line, node->column);
    else if (strcmp(node->name, "==") == 0)   emitByte(compiler, OP_EQUAL, node->line, node->column);
    else if (strcmp(node->name, "!=") == 0)   { emitByte(compiler, OP_EQUAL, node->line, node->column); emitByte(compiler, OP_NOT, node->line, node->column); }
    else if (strcmp(node->name, ">") == 0)    emitByte(compiler, OP_GREATER, node->line, node->column);
    else if (strcmp(node->name, "<") == 0)    emitByte(compiler, OP_LESS, node->line, node->column);
    else if (strcmp(node->name, ">=") == 0)   { emitByte(compiler, OP_LESS, node->line, node->column); emitByte(compiler, OP_NOT, node->line, node->column); }
    else if (strcmp(node->name, "<=") == 0)   { emitByte(compiler, OP_GREATER, node->line, node->column); emitByte(compiler, OP_NOT, node->line, node->column); }
}

static void compileNode(Compiler* compiler, ASTNode* node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_NUMBER: {
            if (node->tokenType == TOKEN_TRUE) {
                emitByte(compiler, OP_TRUE, node->line, node->column);
            } else if (node->tokenType == TOKEN_FALSE) {
                emitByte(compiler, OP_FALSE, node->line, node->column);
            } else if (node->tokenType == TOKEN_FLOAT) {
                emitConstant(compiler, makeFloat(node->floatValue), node->line, node->column);
            } else {
                emitConstant(compiler, makeInt(node->value), node->line, node->column);
            }
            break;
        }
        case NODE_STRING: {
            Value v = makeStringOwned(node->name);
            v.stringOwnership = STRING_OWNERSHIP_CHUNK;
            emitConstant(compiler, v, node->line, node->column);
            break;
        }
        case NODE_NULL: {
            emitByte(compiler, OP_NULL, node->line, node->column);
            break;
        }
        case NODE_BINARY_OP: {
            if (node->left == NULL) {
                compileNode(compiler, node->right);
                if (strcmp(node->name, "-") == 0) emitByte(compiler, OP_NEGATE, node->line, node->column);
                else if (strcmp(node->name, "!") == 0) emitByte(compiler, OP_NOT, node->line, node->column);
            } else {
                compileBinaryOp(compiler, node);
            }
            break;
        }
        case NODE_VAR_DECL: {
            if (node->left) {
                compileNode(compiler, node->left);
            } else {
                emitByte(compiler, OP_NULL, node->line, node->column);
            }
            if (compiler->scopeDepth > 0) {
                Token t = {0};
                t.lexeme = node->name;
                t.ownsLexeme = false;
                addLocal(compiler, t);
                markInitialized(compiler);
            } else {
                Value v = makeStringOwned(node->name);
                v.stringOwnership = STRING_OWNERSHIP_CHUNK;
                uint8_t global = makeConstant(compiler, v);
                emitBytes(compiler, OP_DEFINE_GLOBAL, global, node->line, node->column);
            }
            break;
        }
        case NODE_IDENTIFIER: {
            int local = resolveLocal(compiler, node->name);
            if (local != -1) {
                emitBytes(compiler, OP_GET_LOCAL, (uint8_t)local, node->line, node->column);
            } else {
                int upvalue = resolveUpvalue(compiler, node->name);
                if (upvalue != -1) {
                    emitBytes(compiler, OP_GET_UPVALUE, (uint8_t)upvalue, node->line, node->column);
                } else {
                    Value v = makeStringOwned(node->name);
                    v.stringOwnership = STRING_OWNERSHIP_CHUNK;
                    uint8_t global = makeConstant(compiler, v);
                    emitBytes(compiler, OP_GET_GLOBAL, global, node->line, node->column);
                }
            }
            break;
        }
        case NODE_ASSIGN: {
            if (node->left->type == NODE_IDENTIFIER) {
                compileNode(compiler, node->right);
                int local = resolveLocal(compiler, node->left->name);
                if (local != -1) {
                    emitBytes(compiler, OP_SET_LOCAL, (uint8_t)local, node->line, node->column);
                } else {
                    int upvalue = resolveUpvalue(compiler, node->left->name);
                    if (upvalue != -1) {
                        emitBytes(compiler, OP_SET_UPVALUE, (uint8_t)upvalue, node->line, node->column);
                    } else {
                        Value v = makeStringOwned(node->left->name);
                        v.stringOwnership = STRING_OWNERSHIP_CHUNK;
                        uint8_t global = makeConstant(compiler, v);
                        emitBytes(compiler, OP_SET_GLOBAL, global, node->line, node->column);
                    }
                }
                break;
            }
            if (node->left->type == NODE_ARRAY_INDEX) {
                compileNode(compiler, node->right);       // value
                compileNode(compiler, node->left->left);  // array
                compileNode(compiler, node->left->right); // index
                emitByte(compiler, OP_SET_INDEX, node->line, node->column);
                break;
            }
            compilerErrorAt(compiler, node ? node->line : 0, node ? node->column : 0,
                            "invalid assignment target");
            compiler->hadError = true;
            break;
        }
        case NODE_AUGMENTED_ASSIGN: {
            if (node->left->type == NODE_IDENTIFIER) {
                int local = resolveLocal(compiler, node->left->name);
                if (local != -1) {
                    emitBytes(compiler, OP_GET_LOCAL, (uint8_t)local, node->line, node->column);
                } else {
                    int upvalue = resolveUpvalue(compiler, node->left->name);
                    if (upvalue != -1) {
                        emitBytes(compiler, OP_GET_UPVALUE, (uint8_t)upvalue, node->line, node->column);
                    } else {
                        Value v = makeStringOwned(node->left->name);
                        v.stringOwnership = STRING_OWNERSHIP_CHUNK;
                        uint8_t global = makeConstant(compiler, v);
                        emitBytes(compiler, OP_GET_GLOBAL, global, node->line, node->column);
                    }
                }
                compileNode(compiler, node->right);
                if (strcmp(node->name, "+") == 0)         emitByte(compiler, OP_ADD, node->line, node->column);
                else if (strcmp(node->name, "-") == 0)    emitByte(compiler, OP_SUBTRACT, node->line, node->column);
                else if (strcmp(node->name, "*") == 0)    emitByte(compiler, OP_MULTIPLY, node->line, node->column);
                else if (strcmp(node->name, "/") == 0)    emitByte(compiler, OP_DIVIDE, node->line, node->column);
                if (local != -1) {
                    emitBytes(compiler, OP_SET_LOCAL, (uint8_t)local, node->line, node->column);
                } else {
                    int upvalue = resolveUpvalue(compiler, node->left->name);
                    if (upvalue != -1) {
                        emitBytes(compiler, OP_SET_UPVALUE, (uint8_t)upvalue, node->line, node->column);
                    } else {
                        Value v = makeStringOwned(node->left->name);
                        v.stringOwnership = STRING_OWNERSHIP_CHUNK;
                        uint8_t global = makeConstant(compiler, v);
                        emitBytes(compiler, OP_SET_GLOBAL, global, node->line, node->column);
                    }
                }
                break;
            }
            if (node->left->type == NODE_ARRAY_INDEX) {
                compileNode(compiler, node->left->left);  // array
                compileNode(compiler, node->left->right); // index
                emitByte(compiler, OP_GET_INDEX, node->line, node->column); // current
                compileNode(compiler, node->right);          // rhs
                if (strcmp(node->name, "+") == 0)         emitByte(compiler, OP_ADD, node->line, node->column);
                else if (strcmp(node->name, "-") == 0)    emitByte(compiler, OP_SUBTRACT, node->line, node->column);
                else if (strcmp(node->name, "*") == 0)    emitByte(compiler, OP_MULTIPLY, node->line, node->column);
                else if (strcmp(node->name, "/") == 0)    emitByte(compiler, OP_DIVIDE, node->line, node->column);
                compileNode(compiler, node->left->left);  // array
                compileNode(compiler, node->left->right); // index
                emitByte(compiler, OP_SET_INDEX, node->line, node->column);
                break;
            }
            compilerErrorAt(compiler, node ? node->line : 0, node ? node->column : 0,
                            "invalid augmented assignment target");
            compiler->hadError = true;
            break;
        }
        case NODE_BLOCK: {
            beginScope(compiler);
            ASTNode* stmt = node->body;
            while (stmt) {
                compileNode(compiler, stmt);
                emitDiscardIfNeeded(compiler, stmt);
                stmt = stmt->next;
            }
            endScope(compiler, node->line, node->column);
            break;
        }
        case NODE_PRINT: {
            compileNode(compiler, node->left);
            emitByte(compiler, OP_PRINT, node->line, node->column);
            break;
        }
        case NODE_ARRAY: {
            uint8_t count = 0;
            ASTNode* elem = node->body;
            while (elem) {
                compileNode(compiler, elem);
                count++;
                elem = elem->next;
            }
            emitBytes(compiler, OP_ARRAY, count, node->line, node->column);
            break;
        }
        case NODE_ARRAY_INDEX: {
            compileNode(compiler, node->left);
            compileNode(compiler, node->right);
            emitByte(compiler, OP_GET_INDEX, node->line, node->column);
            break;
        }
        case NODE_RANGE: {
            compileNode(compiler, node->left);
            compileNode(compiler, node->right);
            emitByte(compiler, OP_RANGE, node->line, node->column);
            break;
        }
        case NODE_FOR: {
            int id = tempVarId++;
            char endName[64];
            snprintf(endName, sizeof(endName), "__for_end_%d", id);

            // Evaluate range expression
            compileNode(compiler, node->left); // range
            emitByte(compiler, OP_RANGE_START, node->line, node->column); // push start
            emitByte(compiler, OP_RANGE_END, node->line, node->column);   // push end

            // Define end temp (top of stack)
            Value endVal = makeStringOwned(endName);
            endVal.stringOwnership = STRING_OWNERSHIP_CHUNK;
            uint8_t endConst = makeConstant(compiler, endVal);
            emitBytes(compiler, OP_DEFINE_GLOBAL, endConst, node->line, node->column);

            // Define loop var with start
            Value loopName = makeStringOwned(node->loopVar);
            loopName.stringOwnership = STRING_OWNERSHIP_CHUNK;
            uint8_t loopConst = makeConstant(compiler, loopName);
            emitBytes(compiler, OP_DEFINE_GLOBAL, loopConst, node->line, node->column);

            // Pop range
            emitByte(compiler, OP_POP, node->line, node->column);

            int loopStart = currentChunk(compiler)->count;
            Loop loop;
            loop.enclosing = compiler->currentLoop;
            loop.start = loopStart;
            loop.scopeDepth = compiler->scopeDepth;
            loop.breakCount = 0;
            loop.continueCount = 0;
            loop.continueTarget = loopStart;
            compiler->currentLoop = &loop;

            // Condition: loopVar < end
            emitBytes(compiler, OP_GET_GLOBAL, loopConst, node->line, node->column);
            emitBytes(compiler, OP_GET_GLOBAL, endConst, node->line, node->column);
            emitByte(compiler, OP_LESS, node->line, node->column);
            int exitJump = emitJump(compiler, OP_JUMP_IF_FALSE, node->line, node->column);
            emitByte(compiler, OP_POP, node->line, node->column);

            // Body
            compileNode(compiler, node->body);

            loop.continueTarget = currentChunk(compiler)->count;

            // Increment loopVar
            emitBytes(compiler, OP_GET_GLOBAL, loopConst, node->line, node->column);
            emitConstant(compiler, makeInt(1), node->line, node->column);
            emitByte(compiler, OP_ADD, node->line, node->column);
            emitBytes(compiler, OP_SET_GLOBAL, loopConst, node->line, node->column);
            emitByte(compiler, OP_POP, node->line, node->column);

            emitLoop(compiler, loopStart, node->line, node->column);
            patchJump(compiler, exitJump);
            emitByte(compiler, OP_POP, node->line, node->column);
            int loopEnd = currentChunk(compiler)->count;
            for (int i = 0; i < loop.breakCount; i++) {
                patchJumpTo(compiler, loop.breakJumps[i], loopEnd);
            }
            for (int i = 0; i < loop.continueCount; i++) {
                patchJumpTo(compiler, loop.continueJumps[i], loop.continueTarget);
            }
            compiler->currentLoop = loop.enclosing;
            break;
        }
        case NODE_FUNCTION: {
            compileNode(compiler, node->left);
            uint8_t argCount = argumentList(compiler, node->body);
            emitBytes(compiler, OP_CALL, argCount, node->line, node->column);
            break;
        }
        case NODE_IF: {
            compileNode(compiler, node->left);
            int thenJump = emitJump(compiler, OP_JUMP_IF_FALSE, node->line, node->column);
            emitByte(compiler, OP_POP, node->line, node->column);
            compileNode(compiler, node->body);
            int elseJump = emitJump(compiler, OP_JUMP, node->line, node->column);
            patchJump(compiler, thenJump);
            emitByte(compiler, OP_POP, node->line, node->column);
            if (node->alternate) {
                compileNode(compiler, node->alternate);
            }
            patchJump(compiler, elseJump);
            break;
        }
        case NODE_WHILE: {
            int loopStart = currentChunk(compiler)->count;
            Loop loop;
            loop.enclosing = compiler->currentLoop;
            loop.start = loopStart;
            loop.scopeDepth = compiler->scopeDepth;
            loop.breakCount = 0;
            loop.continueCount = 0;
            loop.continueTarget = loopStart;
            compiler->currentLoop = &loop;

            compileNode(compiler, node->left);
            int exitJump = emitJump(compiler, OP_JUMP_IF_FALSE, node->line, node->column);
            emitByte(compiler, OP_POP, node->line, node->column);
            compileNode(compiler, node->body);
            loop.continueTarget = currentChunk(compiler)->count;
            emitLoop(compiler, loopStart, node->line, node->column);
            patchJump(compiler, exitJump);
            emitByte(compiler, OP_POP, node->line, node->column);
            int loopEnd = currentChunk(compiler)->count;
            for (int i = 0; i < loop.breakCount; i++) {
                patchJumpTo(compiler, loop.breakJumps[i], loopEnd);
            }
            for (int i = 0; i < loop.continueCount; i++) {
                patchJumpTo(compiler, loop.continueJumps[i], loop.continueTarget);
            }
            compiler->currentLoop = loop.enclosing;
            break;
        }
        case NODE_FUNCTION_DEF: {
            if (compiler->scopeDepth > 0) {
                Token t = {0};
                t.lexeme = node->name;
                t.ownsLexeme = false;
                addLocal(compiler, t);
                markInitialized(compiler);
                compileFunction(compiler, node);
            } else {
                compileFunction(compiler, node);
                Value v = makeStringOwned(node->name);
                v.stringOwnership = STRING_OWNERSHIP_CHUNK;
                uint8_t global = makeConstant(compiler, v);
                emitBytes(compiler, OP_DEFINE_GLOBAL, global, node->line, node->column);
            }
            break;
        }
        case NODE_RETURN: {
            if (node->left) {
                compileNode(compiler, node->left);
            } else {
                emitByte(compiler, OP_NULL, node->line, node->column);
            }
            emitByte(compiler, OP_RETURN, node->line, node->column);
            break;
        }
        case NODE_BREAK: {
            if (!compiler->currentLoop) {
                compilerErrorAt(compiler, node->line, node->column,
                                "'break' used outside of loop");
                compiler->hadError = true;
                break;
            }
            emitLoopCleanup(compiler, compiler->currentLoop->scopeDepth, node->line, node->column);
            if (compiler->currentLoop->breakCount >= 256) {
                compilerErrorAt(compiler, node ? node->line : 0, node ? node->column : 0,
                                "too many break statements in loop");
                compiler->hadError = true;
                break;
            }
            compiler->currentLoop->breakJumps[compiler->currentLoop->breakCount++] =
                emitJump(compiler, OP_JUMP, node->line, node->column);
            break;
        }
        case NODE_CONTINUE: {
            if (!compiler->currentLoop) {
                compilerErrorAt(compiler, node->line, node->column,
                                "'continue' used outside of loop");
                compiler->hadError = true;
                break;
            }
            emitLoopCleanup(compiler, compiler->currentLoop->scopeDepth, node->line, node->column);
            if (compiler->currentLoop->continueCount >= 256) {
                compilerErrorAt(compiler, node ? node->line : 0, node ? node->column : 0,
                                "too many continue statements in loop");
                compiler->hadError = true;
                break;
            }
            compiler->currentLoop->continueJumps[compiler->currentLoop->continueCount++] =
                emitJump(compiler, OP_JUMP, node->line, node->column);
            break;
        }
        default:
            compilerErrorAt(compiler, node ? node->line : 0, node ? node->column : 0,
                            "unknown AST node type %d", node->type);
            compiler->hadError = true;
            break;
    }
}

bool compile(ASTNode* program, Chunk* chunk) {
    Compiler compiler;
    initCompiler(&compiler, chunk, NULL);

    ASTNode* stmt = program;
    while (stmt && !compiler.hadError) {
        compileNode(&compiler, stmt);
        emitDiscardIfNeeded(&compiler, stmt);

        stmt = stmt->next;
    }

    emitByte(&compiler, OP_RETURN, 0, 0);

    return !compiler.hadError;
}
