// eval.c
// Created by Derek Roper on 3/2/26.
//

#include "eval.h"
#include "ast.h"
#include <stddef.h>
#include <stdio.h>
#include "env.h"

int evaluate(ASTNode* node, Env* env) {
    if (node == NULL) {
        return 0;
    }

    switch (node->type) {
        case NODE_NUMBER:
            return node->value;
        case NODE_IDENTIFIER:
            if (node->name) {
                int value = env_get(env, node->name);
                return value;
            } else {
                return 0;
            }
        case NODE_BINARY_OP:
            int left_value = evaluate(node->left, env);
            int right_value = evaluate(node->right, env);
            switch (node->op) {
                case '+':
                    return left_value + right_value;
                case '-':
                    return left_value - right_value;
                default:
                    printf("Unsupported binary operation: '%s'\n", node->name);
                    return 0;
            }
        case NODE_UNKNOWN:
            printf("Unknown token: %s\n", node->name);
            return 0;
        default:
            printf("Unsupported node type: %d\n", node->type);
            return 0;
    }
}
