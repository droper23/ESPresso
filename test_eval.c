// // test_eval.c
// // Created by Derek Roper on 3/2/26.
// //
//
// #include <stdio.h>
// #include <_string.h>
//
// #include "eval.h"
// #include "env.h"
// #include "ast.h"
// #include "parser.h"
//
// int main() {
//     // Create global environment
//     Env* global = create_environment(NULL);
//
//     // AST for:
//     // x = 1
//     ASTNode* assign_x = makeNode(NODE_ASSIGN);
//     assign_x->left = makeNode(NODE_IDENTIFIER);
//     assign_x->left->name = strdup("x");
//     assign_x->right = makeNode(NODE_NUMBER);
//     assign_x->right->value = 1;
//
//     // AST for block:
//     // {
//     //    x = 2
//     //    y = 3
//     // }
//     ASTNode* block = makeNode(NODE_BLOCK);
//
//     // stmt1: x = 2
//     ASTNode* stmt1 = makeNode(NODE_ASSIGN);
//     stmt1->left = makeNode(NODE_IDENTIFIER);
//     stmt1->left->name = strdup("x");
//     stmt1->right = makeNode(NODE_NUMBER);
//     stmt1->right->value = 2;
//
//     // stmt2: y = 3
//     ASTNode* stmt2 = makeNode(NODE_ASSIGN);
//     stmt2->left = makeNode(NODE_IDENTIFIER);
//     stmt2->left->name = strdup("y");
//     stmt2->right = makeNode(NODE_NUMBER);
//     stmt2->right->value = 3;
//
//     // Link statements
//     stmt1->next = stmt2;
//     stmt2->next = NULL;
//
//     block->body = stmt1;
//
//     // Evaluate global assignment
//     evaluate(assign_x, global);
//
//     // Evaluate block
//     evaluate(block, global);
//
//     // Check results
//     printf("x = %d\n", env_get(global, "x")); // Should be 2
//     printf("y = %d\n", env_get(global, "y")); // Should be 3
//
//     // Clean up
//     freeAST(assign_x);
//     freeAST(block);
//     free_environment(global);
//
//     return 0;
// }
