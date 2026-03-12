// eval.c
// Created by Derek Roper on 3/2/26.
//

#include "eval.h"
#include "ast.h"
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"

static int isTruthy(Value v) {
    switch (v.type) {
        case VALUE_INT:    return v.data.intValue != 0;
        case VALUE_FLOAT:  return v.data.floatValue != 0.0f;
        case VALUE_BOOL:   return v.data.boolValue != 0;
        case VALUE_STRING: return v.data.stringValue != NULL && v.data.stringValue[0] != '\0';
        case VALUE_NIL:    return 0;
        default:           return 0;
    }
}

static void printValue(Value v) {
    switch (v.type) {
        case VALUE_INT:    printf("%d", v.data.intValue);    break;
        case VALUE_FLOAT:  printf("%g", v.data.floatValue);  break;
        case VALUE_STRING: printf("%s", v.data.stringValue); break;
        case VALUE_BOOL:   printf("%s", v.data.boolValue ? "true" : "false"); break;
        case VALUE_NIL:    printf("nil");                    break;
        default:           printf("<unknown>");              break;
    }
}

static Value applyBinaryOp(const char* op, Value left, Value right) {
    if (strcmp(op, "+") == 0 &&
        left.type == VALUE_STRING && right.type == VALUE_STRING) {
        size_t len = strlen(left.data.stringValue) + strlen(right.data.stringValue) + 1;
        char* buf = malloc(len);
        strcpy(buf, left.data.stringValue);
        strcat(buf, right.data.stringValue);
        Value result;
        result.type = VALUE_STRING;
        result.data.stringValue = buf;
        return result;
    }

    int useFloat = (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT);
    float l = (left.type  == VALUE_FLOAT) ? left.data.floatValue  : (float)left.data.intValue;
    float r = (right.type == VALUE_FLOAT) ? right.data.floatValue : (float)right.data.intValue;

    if (strcmp(op, "+")  == 0) return useFloat ? makeFloat(l + r) : makeInt((int)(l + r));
    if (strcmp(op, "-")  == 0) return useFloat ? makeFloat(l - r) : makeInt((int)(l - r));
    if (strcmp(op, "*")  == 0) return useFloat ? makeFloat(l * r) : makeInt((int)(l * r));
    if (strcmp(op, "/")  == 0) {
        if (r == 0.0f) { printf("Error: division by zero\n"); return makeNil(); }
        return useFloat ? makeFloat(l / r) : makeInt((int)(l / r));
    }
    if (strcmp(op, ">")  == 0) return makeBool(l >  r);
    if (strcmp(op, "<")  == 0) return makeBool(l <  r);
    if (strcmp(op, "==") == 0) return makeBool(l == r);
    if (strcmp(op, "!=") == 0) return makeBool(l != r);
    if (strcmp(op, "<=") == 0) return makeBool(l <= r);
    if (strcmp(op, ">=") == 0) return makeBool(l >= r);

    printf("Unsupported binary operation: '%s'\n", op);
    return makeNil();
}

Value evaluate(ASTNode* node, Env* env) {
    if (node == NULL) {
        return makeNil();
    }

    switch (node->type) {
        case NODE_NUMBER: {
            return makeInt(node->value);
        }

        case NODE_STRING: {
            return makeString(node->name);
        }

        case NODE_IDENTIFIER: {
            if (node->name) {
                return env_get(env, node->name);
            }
            return makeNil();
        }

        case NODE_IF: {
            Value cond = evaluate(node->left, env);
            if (isTruthy(cond)) {
                evaluate(node->body, env);
            }
            return makeNil();
        }

        case NODE_WHILE: {
            while (isTruthy(evaluate(node->left, env))) {
                evaluate(node->body, env);
            }
            return makeNil();
        }

        case NODE_BINARY_OP: {
            Value left  = evaluate(node->left,  env);
            Value right = evaluate(node->right, env);

            if (node->name == NULL) {
                printf("Unsupported binary operation\n");
                return makeNil();
            }

            return applyBinaryOp(node->name, left, right);
        }

        case NODE_ASSIGN: {
            Value val = evaluate(node->right, env);
            env_set(env, node->left->name, val);
            return val;
        }

        case NODE_BLOCK: {
            ASTNode* stmt = node->body;
            Value result = makeNil();
            while (stmt != NULL) {
                result = evaluate(stmt, env);
                stmt = stmt->next;
            }
            return result;
        }

        case NODE_PRINT: {
            Value val = evaluate(node->left, env);
            printValue(val);
            printf("\n");
            return val;
        }

        case NODE_FUNCTION: {
            if (node->name == NULL) return makeNil();

            int isPrint = (strcmp(node->name, "print") == 0);
            int isLog   = (strcmp(node->name, "log")   == 0);

            if (isPrint || isLog) {
                ASTNode* arg = node->body;
                int first = 1;
                Value lastVal = makeNil();
                while (arg != NULL) {
                    if (!first) printf(" ");
                    first = 0;
                    lastVal = evaluate(arg, env);
                    printValue(lastVal);
                    arg = arg->next;
                }
                printf("\n");
                return lastVal;
            }

            printf("Unknown function: '%s'\n", node->name);
            return makeNil();
        }

        case NODE_UNKNOWN: {
            printf("Unknown token: %s\n", node->name);
            return makeNil();
        }

        default:
            printf("Unsupported node type: %d\n", node->type);
            return makeNil();
    }
}

void freeAST(ASTNode* node) {
    if (!node) return;

    freeAST(node->left);
    freeAST(node->right);

    if (node->type == NODE_BLOCK || node->type == NODE_FUNCTION) {
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