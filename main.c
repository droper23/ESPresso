// main.c

#include <stdio.h>
#include "env.h"
#include "eval.h"
#include "ast.h"
#include <stdio.h>
#include <_string.h>

#include "parser.h"

int main() {
    Env* global = create_environment(NULL);

    // simulate parsing: x = 5
    ASTNode* assign_x = makeNode(NODE_ASSIGN);
    assign_x->left = makeNode(NODE_IDENTIFIER);
    assign_x->left->name = strdup("x");
    assign_x->right = makeNode(NODE_NUMBER);
    assign_x->right->value = 5;

    int result = evaluate(assign_x, global);
    printf("x = %d\n", env_get(global, "x"));  // should print 5

    // simulate: y = x + 3
    ASTNode* add = makeNode(NODE_BINARY_OP);
    add->left = makeNode(NODE_IDENTIFIER);
    add->left->name = strdup("x");
    add->right = makeNode(NODE_NUMBER);
    add->right->value = 3;
    add->op = '+';

    ASTNode* assign_y = makeNode(NODE_ASSIGN);
    assign_y->left = makeNode(NODE_IDENTIFIER);
    assign_y->left->name = strdup("y");
    assign_y->right = add;

    result = evaluate(assign_y, global);
    printf("y = %d\n", env_get(global, "y"));  // should print 8

    // free everything
    freeAST(assign_x);
    freeAST(assign_y);
    free_environment(global);

    return 0;
}
