// compiler.c
// Created by Derek Roper on 3/16/26.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "parser.h"
#include "chunk.h"

typedef struct {
    Token name;
    int depth;
} Local;

typedef struct Compiler {
    struct Compiler* enclosing;
    Chunk* chunk;

    Local locals[256];
    int localCount;
    int scopeDepth;
    bool hadError;
} Compiler;

static void initCompiler(Compiler* compiler, Chunk* chunk, struct Compiler* enclosing) {
    compiler->enclosing = enclosing;
    compiler->chunk = chunk;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->hadError = false;

    Local* local = &compiler->locals[compiler->localCount++];
    local->depth = 0;
    local->name.lexeme = "";
}

static Chunk* currentChunk(Compiler* compiler) {
    return compiler->chunk;
}

static void emitByte(Compiler* compiler, uint8_t byte, int line) {
    writeChunk(currentChunk(compiler), byte, line);
}

static void emitBytes(Compiler* compiler, uint8_t byte1, uint8_t byte2, int line) {
    emitByte(compiler, byte1, line);
    emitByte(compiler, byte2, line);
}

static uint8_t makeConstant(Compiler* compiler, Value value) {
    int constant = addConstant(currentChunk(compiler), value);
    if (constant > UINT8_MAX) {
        compiler->hadError = true;
        printf("Error: Too many constants in one chunk.\n");
        return 0;
    }
    return (uint8_t)constant;
}

static void emitConstant(Compiler* compiler, Value value, int line) {
    emitBytes(compiler, OP_CONSTANT, makeConstant(compiler, value), line);
}

static int emitJump(Compiler* compiler, uint8_t instruction, int line) {
    emitByte(compiler, instruction, line);
    emitByte(compiler, 0xff, line);
    emitByte(compiler, 0xff, line);
    return currentChunk(compiler)->count - 2;
}

static void patchJump(Compiler* compiler, int offset) {
    int jump = currentChunk(compiler)->count - offset - 2;
    if (jump > UINT16_MAX) {
        compiler->hadError = true;
        printf("Error: Too much code to jump over.\n");
    }
    currentChunk(compiler)->code[offset] = (jump >> 8) & 0xff;
    currentChunk(compiler)->code[offset + 1] = jump & 0xff;
}

static void emitLoop(Compiler* compiler, int loopStart, int line) {
    emitByte(compiler, OP_LOOP, line);
    int offset = currentChunk(compiler)->count - loopStart + 2;
    if (offset > UINT16_MAX) {
        compiler->hadError = true;
        printf("Error: Loop body too large.\n");
    }
    emitByte(compiler, (offset >> 8) & 0xff, line);
    emitByte(compiler, offset & 0xff, line);
}

static void beginScope(Compiler* compiler) {
    compiler->scopeDepth++;
}

static void endScope(Compiler* compiler, int line) {
    while (compiler->localCount > 0 &&
            compiler->locals[compiler->localCount - 1].depth >
            compiler->scopeDepth) {
        emitByte(compiler, OP_POP, line);
        compiler->localCount--;
    }
}

static void addLocal(Compiler* compiler, Token name) {
    if (compiler->localCount == 256) {
        printf("Error: Too many local variables in function.\n");
        compiler->hadError = true;
        return;
    }
    Local* local = &compiler->locals[compiler->localCount++];
    local->name = name;
    local->depth = -1; // Mark as uninitialized
}

static void markInitialized(Compiler* compiler) {
    if (compiler->scopeDepth == 0) return;
    compiler->locals[compiler->localCount - 1].depth = compiler->scopeDepth;
}

static void compileNode(Compiler* compiler, ASTNode* node);

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

static void compileBinaryOp(Compiler* compiler, ASTNode* node) {
    if (strcmp(node->name, "and") == 0) {
        compileNode(compiler, node->left);
        int endJump = emitJump(compiler, OP_JUMP_IF_FALSE, node->line);
        emitByte(compiler, OP_POP, node->line);
        compileNode(compiler, node->right);
        patchJump(compiler, endJump);
        return;
    } else if (strcmp(node->name, "or") == 0) {
        compileNode(compiler, node->left);
        int elseJump = emitJump(compiler, OP_JUMP_IF_FALSE, node->line);
        int endJump = emitJump(compiler, OP_JUMP, node->line);
        patchJump(compiler, elseJump);
        emitByte(compiler, OP_POP, node->line);
        compileNode(compiler, node->right);
        patchJump(compiler, endJump);
        return;
    }

    compileNode(compiler, node->left);
    compileNode(compiler, node->right);

    if (strcmp(node->name, "+") == 0)         emitByte(compiler, OP_ADD, node->line);
    else if (strcmp(node->name, "-") == 0)    emitByte(compiler, OP_SUBTRACT, node->line);
    else if (strcmp(node->name, "*") == 0)    emitByte(compiler, OP_MULTIPLY, node->line);
    else if (strcmp(node->name, "/") == 0)    emitByte(compiler, OP_DIVIDE, node->line);
    else if (strcmp(node->name, "==") == 0)   emitByte(compiler, OP_EQUAL, node->line);
    else if (strcmp(node->name, "!=") == 0)   { emitByte(compiler, OP_EQUAL, node->line); emitByte(compiler, OP_NOT, node->line); }
    else if (strcmp(node->name, ">") == 0)    emitByte(compiler, OP_GREATER, node->line);
    else if (strcmp(node->name, "<") == 0)    emitByte(compiler, OP_LESS, node->line);
    else if (strcmp(node->name, ">=") == 0)   { emitByte(compiler, OP_LESS, node->line); emitByte(compiler, OP_NOT, node->line); }
    else if (strcmp(node->name, "<=") == 0)   { emitByte(compiler, OP_GREATER, node->line); emitByte(compiler, OP_NOT, node->line); }
}

static void compileNode(Compiler* compiler, ASTNode* node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_NUMBER: {
            if (node->tokenType == TOKEN_TRUE) {
                emitByte(compiler, OP_TRUE, node->line);
            } else if (node->tokenType == TOKEN_FALSE) {
                emitByte(compiler, OP_FALSE, node->line);
            } else if (node->tokenType == TOKEN_FLOAT) {
                emitConstant(compiler, makeFloat(node->floatValue), node->line);
            } else {
                emitConstant(compiler, makeInt(node->value), node->line);
            }
            break;
        }
        case NODE_STRING: {
            Value v = makeStringOwned(node->name);
            v.stringOwnership = STRING_OWNERSHIP_CHUNK;
            emitConstant(compiler, v, node->line);
            break;
        }
        case NODE_NULL: {
            emitByte(compiler, OP_NULL, node->line);
            break;
        }
        case NODE_BINARY_OP: {
            if (node->left == NULL) {
                compileNode(compiler, node->right);
                if (strcmp(node->name, "-") == 0) emitByte(compiler, OP_NEGATE, node->line);
                else if (strcmp(node->name, "!") == 0) emitByte(compiler, OP_NOT, node->line);
            } else {
                compileBinaryOp(compiler, node);
            }
            break;
        }
        case NODE_VAR_DECL: {
            if (node->left) {
                compileNode(compiler, node->left);
            } else {
                emitByte(compiler, OP_NULL, node->line);
            }
            Value v = makeStringOwned(node->name);
            v.stringOwnership = STRING_OWNERSHIP_CHUNK;
            uint8_t global = makeConstant(compiler, v);
            emitBytes(compiler, OP_DEFINE_GLOBAL, global, node->line);
            break;
        }
        case NODE_IDENTIFIER: {
            Value v = makeStringOwned(node->name);
            v.stringOwnership = STRING_OWNERSHIP_CHUNK;
            uint8_t global = makeConstant(compiler, v);
            emitBytes(compiler, OP_GET_GLOBAL, global, node->line);
            break;
        }
        case NODE_ASSIGN: {
            if (node->left->type != NODE_IDENTIFIER) {
                printf("Error: Invalid assignment target.\n");
                compiler->hadError = true;
                break;
            }
            compileNode(compiler, node->right);
            Value v = makeStringOwned(node->left->name);
            v.stringOwnership = STRING_OWNERSHIP_CHUNK;
            uint8_t global = makeConstant(compiler, v);
            emitBytes(compiler, OP_SET_GLOBAL, global, node->line);
            break;
        }
        case NODE_BLOCK: {
            ASTNode* stmt = node->body;
            while (stmt) {
                compileNode(compiler, stmt);
                stmt = stmt->next;
            }
            break;
        }
        case NODE_PRINT: {
            compileNode(compiler, node->left);
            emitByte(compiler, OP_PRINT, node->line);
            break;
        }
        case NODE_FUNCTION: {
            compileNode(compiler, node->left);
            uint8_t argCount = argumentList(compiler, node->body);
            emitBytes(compiler, OP_CALL, argCount, node->line);
            break;
        }
        case NODE_IF: {
            compileNode(compiler, node->left);
            int thenJump = emitJump(compiler, OP_JUMP_IF_FALSE, node->line);
            emitByte(compiler, OP_POP, node->line);
            compileNode(compiler, node->body);
            int elseJump = emitJump(compiler, OP_JUMP, node->line);
            patchJump(compiler, thenJump);
            emitByte(compiler, OP_POP, node->line);
            if (node->alternate) {
                compileNode(compiler, node->alternate);
            }
            patchJump(compiler, elseJump);
            break;
        }
        case NODE_WHILE: {
            int loopStart = currentChunk(compiler)->count;
            compileNode(compiler, node->left);
            int exitJump = emitJump(compiler, OP_JUMP_IF_FALSE, node->line);
            emitByte(compiler, OP_POP, node->line);
            compileNode(compiler, node->body);
            emitLoop(compiler, loopStart, node->line);
            patchJump(compiler, exitJump);
            emitByte(compiler, OP_POP, node->line);
            break;
        }
        case NODE_AUGMENTED_ASSIGN:
        case NODE_ARRAY:
        case NODE_ARRAY_INDEX:
        case NODE_FOR:
        case NODE_FUNCTION_DEF:
        case NODE_RETURN:
        case NODE_RANGE:
            printf("Compiler Error: AST node type %d not supported in VM yet.\n", node->type);
            compiler->hadError = true;
            break;
        default:
            printf("Compiler Error: Unknown AST node type %d\n", node->type);
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

        switch (stmt->type) {
            case NODE_ASSIGN:
            case NODE_AUGMENTED_ASSIGN:
            case NODE_BINARY_OP:
            case NODE_NUMBER:
            case NODE_STRING:
            case NODE_IDENTIFIER:
            case NODE_FUNCTION:
                emitByte(&compiler, OP_POP, stmt->line);
                break;
            default:
                break;
        }

        stmt = stmt->next;
    }

    emitByte(&compiler, OP_RETURN, 0);

    return !compiler.hadError;
}
