// main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "eval.h"
#include "env.h"
#include "chunk.h"
#include "compiler.h"
#include "vm.h"

Value nativePrint(int argCount, Value* args) {
    for (int i = 0; i < argCount; i++) {
        printValue(args[i]);
        if (i < argCount - 1) printf(" ");
    }
    printf("\n");
    return makeNull();
}

int main(int argc, char* argv[]) {
    int useInterp = 0;
    int trace = 0;
    const char* filename = NULL;

    if (argc < 2) {
        printf("Usage: %s [--interp] [--trace] <file.espr>\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--interp") == 0) {
            useInterp = 1;
        } else if (strcmp(argv[i], "--trace") == 0) {
            trace = 1;
        } else {
            filename = argv[i];
        }
    }

    if (!filename) {
        printf("Usage: %s [--interp] [--trace] <file.espr>\n", argv[0]);
        return 1;
    }

    // Read file
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("fopen");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* source = malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);

    // Initialize lexer and parser
    Lexer lexer;
    initLexer(&lexer, source);

    Parser parser;
    initParser(&parser, &lexer);

    // Parse statements into AST
    ASTNode* program = NULL;
    ASTNode* last = NULL;
    while (parser.current.type != TOKEN_EOF) {
        ASTNode* stmt = parseStatement(&parser);
        if (!program) {
            program = stmt;
        } else {
            last->next = stmt;
        }
        last = stmt;
    }

    // Create global environment
    Env* global = create_environment(NULL);
    env_define(global, "print", makeNative(nativePrint));
    env_define(global, "log", makeNative(nativePrint)); // optional alias

    if (useInterp) {
        // Evaluate the program with AST interpreter
        ASTNode* stmt = program;
        while (stmt) {
            evaluate(stmt, global);
            stmt = stmt->next;
        }
    } else {
        // Compile and run via VM
        Chunk chunk;
        initChunk(&chunk);
        if (!compile(program, &chunk)) {
            freeChunk(&chunk);
            freeAST(program);
            free_environment(global);
            free(source);
            return 1;
        }
        vmSetTrace(trace);
        InterpretResult result = interpret(&chunk, global);
        freeChunk(&chunk);
        if (result != INTERPRET_OK) {
            freeAST(program);
            free_environment(global);
            free(source);
            return 1;
        }
    }

    // Cleanup
    freeAST(program);
    free_environment(global);
    free(source);

    return 0;
}
