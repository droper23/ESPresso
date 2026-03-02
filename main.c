#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include "ast_utils.h"

void printNode(ASTNode* node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth; i++) {
        printf("  ");
    }

    printf("%s", nodeTypeToString(node->type));

    switch(node->type) {
        case NODE_NUMBER:
            printf(" %d", node->value);
            break;
        case NODE_IDENTIFIER:
            printf(" %s", node->name);
            break;
        case NODE_BINARY_OP:
            printf(" '%c'", node->op);
            break;
        default:
            break;
    }
    printf("\n");

    printNode(node->left, depth + 1);
    printNode(node->right, depth + 1);
}

int main(void) {
    const char* source = "print 17 + (9 - 3)";
    Lexer lexer;
    lexer.current = source;
    lexer.start = source;

    Parser parser;
    initParser(&parser, &lexer);

    ASTNode* root = parseStatement(&parser);

    printNode(root, 0);
}