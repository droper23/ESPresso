// main.c

#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "eval.h"
#include "env.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file.espr>\n", argv[0]);
        return 1;
    }

    // Read file
    FILE* f = fopen(argv[1], "r");
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

    // Evaluate the program
    ASTNode* stmt = program;
    while (stmt) {
        evaluate(stmt, global);
        stmt = stmt->next;
    }

    // Cleanup
    freeAST(program);
    free_environment(global);
    free(source);

    return 0;
}