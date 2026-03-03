// main.c

#include <stdio.h>
#include <stdlib.h>
#include <_string.h>
#include "env.h"
#include "eval.h"
#include "ast.h"
#include "parser.h"

// Helper to link nodes in a block
void link_next(ASTNode* first, ASTNode* second) {
    first->next = second;
}

int main() {
    // Create global environment
    Env* global = create_environment(NULL);

    // Build AST manually: block { x = 1; y = x + 2; }
    ASTNode* assign_x = makeNode(NODE_ASSIGN);
    assign_x->left = makeNode(NODE_IDENTIFIER);
    assign_x->left->name = strdup("x");
    assign_x->right = makeNode(NODE_NUMBER);
    assign_x->right->value = 1;

    ASTNode* assign_y = makeNode(NODE_ASSIGN);
    assign_y->left = makeNode(NODE_IDENTIFIER);
    assign_y->left->name = strdup("y");
    assign_y->right = makeNode(NODE_BINARY_OP);
    assign_y->right->op = '+';
    assign_y->right->left = makeNode(NODE_IDENTIFIER);
    assign_y->right->left->name = strdup("x");
    assign_y->right->right = makeNode(NODE_NUMBER);
    assign_y->right->right->value = 2;

    link_next(assign_x, assign_y);

    ASTNode* block = makeNode(NODE_BLOCK);
    block->body = assign_x;

    // Evaluate block
    int result = evaluate(block, global);
    printf("Block result: %d\n", result);

    // Check values in global environment
    printf("global x = %d\n", env_get(global, "x"));
    printf("global y = %d\n", env_get(global, "y"));

    // Free everything
    free_environment(global);
    free(block->body->left->name);
    free(block->body->right->left->name);
    free(block->body->right->right);
    free(block->body->right->left);
    free(block->body->right);
    free(block->body->left);
    free(block->body);
    free(block);

    return 0;
}