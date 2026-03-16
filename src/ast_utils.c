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
        case NODE_IF:           return "IF";
        case NODE_WHILE:        return "WHILE";
        case NODE_FOR:          return "FOR";
        case NODE_FUNCTION:     return "FUNCTION_CALL";
        case NODE_RETURN:       return "RETURN";
        case NODE_FUNCTION_DEF: return "FUNCTION_DEF";
        case NODE_VAR_DECL:     return "VAR_DECL";
        case NODE_LOGICAL_AND:  return "LOGICAL_AND";
        case NODE_LOGICAL_OR:   return "LOGICAL_OR";
        case NODE_NIL:          return "NIL";
        case NODE_ASSIGN:       return "ASSIGN";
        default:                return "OTHER";
    }
}