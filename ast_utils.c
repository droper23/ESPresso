// ast_utils.c
// Created by Derek Roper on 3/2/26.
//

#include "ast_utils.h"

const char* nodeTypeToString(NodeType type) {
    switch(type) {
        case NODE_NUMBER:       return "NUMBER";
        case NODE_BINARY_OP:    return "BINARY_OP";
        case NODE_PRINT:        return "PRINT";
        case NODE_IDENTIFIER:   return "IDENTIFIER";
        case NODE_UNKNOWN:      return "UNKNOWN";
        case NODE_BLOCK:        return "BLOCK";
        default:                return "OTHER";
    }
}