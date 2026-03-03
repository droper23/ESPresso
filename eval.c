// eval.c
// Created by Derek Roper on 3/2/26.
//

#include "eval.h"
#include "ast.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "env.h"

int evaluate(ASTNode* node, Env* env) {
    if (node == NULL) {
        return 0;
    }

    switch (node->type) {
        case NODE_NUMBER: {
            return node->value;
        }
        case NODE_IDENTIFIER: {
            if (node->name) {
                int value = env_get(env, node->name);
                return value;
            } else {
                return 0;
            }
        }
        case NODE_BINARY_OP: {
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
        }
        case NODE_UNKNOWN: {
            printf("Unknown token: %s\n", node->name);
            return 0;
        }
        case NODE_BLOCK: {
            Env* local = create_environment(env);

            ASTNode* stmt = node->body;
            int result = 0;

            while (stmt != NULL) {
                result = evaluate(stmt, local);
                stmt = stmt->next;
            }

            free_environment(local);
            return result;
        }
        case NODE_ASSIGN: {
            int value = evaluate(node->right, env);
            env_set(env, node->left->name, value);
            return value;
        }

        default:
            printf("Unsupported node type: %d\n", node->type);
            return 0;
    }
}

void freeAST(ASTNode* node) {
    if (!node) return;

    freeAST(node->left);
    freeAST(node->right);

    if (node->type == NODE_BLOCK) {
        ASTNode* stmt = node->body;
        while (stmt) {
            ASTNode* next = stmt->next;
            freeAST(stmt);
            stmt = next;
        }
    }

    if (node->name) free(node->name);
    free(node);
}