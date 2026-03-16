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


static Value applyBinaryOp(const char* op, Value left, Value right) {
    if (strcmp(op, "+") == 0 &&
        (left.type == VALUE_STRING || right.type == VALUE_STRING)) {
        char* lStr = valueToString(left);
        char* rStr = valueToString(right);
        
        size_t len = strlen(lStr) + strlen(rStr) + 1;
        char* buf = malloc(len);
        strcpy(buf, lStr);
        strcat(buf, rStr);
        
        free(lStr);
        free(rStr);

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
        if (r == 0.0f) { printf("Error: division by zero at [Line %d, Col %d]\n", left.line, left.column); return makeNull(); }
        return useFloat ? makeFloat(l / r) : makeInt((int)(l / r));
    }
    if (strcmp(op, ">")  == 0) return makeBool(l >  r);
    if (strcmp(op, "<")  == 0) return makeBool(l <  r);
    if (strcmp(op, "==") == 0) return makeBool(l == r);
    if (strcmp(op, "!=") == 0) return makeBool(l != r);
    if (strcmp(op, "<=") == 0) return makeBool(l <= r);
    if (strcmp(op, ">=") == 0) return makeBool(l >= r);

    printf("Unsupported binary operation: '%s'\n", op);
    return makeNull();
}

static Value evaluate_internal(ASTNode* node, Env* env);

Value evaluate(ASTNode* node, Env* env) {
    if (node == NULL) return makeNull();
    Value v = evaluate_internal(node, env);
    if (v.type != VALUE_RETURN) {
        v.line = node->line;
        v.column = node->column;
    }
    return v;
}

static Value evaluate_internal(ASTNode* node, Env* env) {
    switch (node->type) {
        case NODE_NUMBER: {
            if (node->tokenType == TOKEN_FLOAT) {
                return makeFloat(node->floatValue);
            }
            return makeInt(node->value);
        }

        case NODE_STRING: {
            return makeString(node->name);
        }

        case NODE_IDENTIFIER: {
            if (node->name) {
                return env_get(env, node->name);
            }
            return makeNull();
        }

        case NODE_NULL: {
            return makeNull();
        }

        case NODE_IF: {
            Value cond = evaluate(node->left, env);
            if (cond.type == VALUE_RETURN) return cond;

            if (isTruthy(cond)) {
                return evaluate(node->body, env);
            } else if (node->alternate) {
                return evaluate(node->alternate, env);
            }
            return makeNull();
        }

        case NODE_WHILE: {
            Value result = makeNull();
            while (1) {
                Value cond = evaluate(node->left, env);
                if (cond.type == VALUE_RETURN) return cond;
                if (!isTruthy(cond)) break;

                result = evaluate(node->body, env);
                if (result.type == VALUE_RETURN) return result;
            }
            return result;
        }

        case NODE_ARRAY: {
            int count = 0;
            ASTNode* elemNode = node->body;
            while (elemNode) {
                count++;
                elemNode = elemNode->next;
            }

            Value* elements = NULL;
            if (count > 0) {
                elements = malloc(sizeof(Value) * count);
                elemNode = node->body;
                for (int i = 0; i < count; i++) {
                    elements[i] = evaluate(elemNode, env);
                    elemNode = elemNode->next;
                }
            }
            return makeArray(count, elements);
        }

        case NODE_ARRAY_INDEX: {
            Value left = evaluate(node->left, env);
            if (left.type == VALUE_RETURN) return left;
            Value right = evaluate(node->right, env);
            if (right.type == VALUE_RETURN) return right;

            if (left.type != VALUE_ARRAY) {
                printf("Error: indexing requires an array, found %d at [Line %d]\n", left.type, node->line);
                return makeNull();
            }
            if (right.type != VALUE_INT) {
                printf("Error: array index must be an integer at [Line %d]\n", node->line);
                return makeNull();
            }

            int idx = right.data.intValue;
            if (idx < 0 || idx >= left.data.arrayValue.count) {
                printf("Error: array index out of bounds: %d at [Line %d]\n", idx, node->line);
                return makeNull();
            }

            return left.data.arrayValue.elements[idx];
        }

        case NODE_RANGE: {
            Value left = evaluate(node->left, env);
            if (left.type == VALUE_RETURN) return left;
            Value right = evaluate(node->right, env);
            if (right.type == VALUE_RETURN) return right;

            if (left.type != VALUE_INT || right.type != VALUE_INT) {
                printf("Error: range bounds must be integers at [Line %d, Col %d]\n", node->line, node->column);
                return makeNull();
            }
            return makeRange(left.data.intValue, right.data.intValue);
        }

        case NODE_FOR: {
            Value rangeVal = evaluate(node->left, env);
            if (rangeVal.type == VALUE_RETURN) return rangeVal;

            if (rangeVal.type != VALUE_RANGE) {
                printf("Error: for loop expects a range at [Line %d, Col %d]\n", node->line, node->column);
                return makeNull();
            }

            int start = rangeVal.data.rangeValue.start;
            int end = rangeVal.data.rangeValue.end;

            Env* forEnv = create_environment(env);
            Value result = makeNull();

            for (int i = start; i < end; i++) {
                env_define(forEnv, node->loopVar, makeInt(i));
                result = evaluate(node->body, forEnv);
                if (result.type == VALUE_RETURN) {
                    free_environment(forEnv);
                    return result;
                }
            }

            free_environment(forEnv);
            return result;
        }

        case NODE_BINARY_OP: {
            if (strcmp(node->name, "&&") == 0 || strcmp(node->name, "and") == 0) {
                Value left = evaluate(node->left, env);
                if (left.type == VALUE_RETURN) return left;
                if (!isTruthy(left)) return left;
                return evaluate(node->right, env);
            }
            if (strcmp(node->name, "||") == 0 || strcmp(node->name, "or") == 0) {
                Value left = evaluate(node->left, env);
                if (left.type == VALUE_RETURN) return left;
                if (isTruthy(left)) return left;
                return evaluate(node->right, env);
            }

            if (strcmp(node->name, "!") == 0) {
                Value right = evaluate(node->right, env);
                if (right.type == VALUE_RETURN) return right;
                return makeBool(!isTruthy(right));
            }
            if (strcmp(node->name, "-") == 0 && node->left == NULL) {
                Value right = evaluate(node->right, env);
                if (right.type == VALUE_RETURN) return right;
                if (right.type == VALUE_FLOAT) return makeFloat(-right.data.floatValue);
                return makeInt(-right.data.intValue);
            }

            Value left  = evaluate(node->left,  env);
            if (left.type == VALUE_RETURN) return left;
            Value right = evaluate(node->right, env);
            if (right.type == VALUE_RETURN) return right;

            if (node->name == NULL) {
                printf("Unsupported binary operation\n");
                return makeNull();
            }

            return applyBinaryOp(node->name, left, right);
        }

        case NODE_ASSIGN: {
            Value val = evaluate(node->right, env);
            if (val.type == VALUE_RETURN) return val;
            
            if (node->left->type == NODE_IDENTIFIER) {
                env_set(env, node->left->name, val);
            } else if (node->left->type == NODE_ARRAY_INDEX) {
                Value arr = evaluate(node->left->left, env);
                Value idx = evaluate(node->left->right, env);
                if (arr.type != VALUE_ARRAY || idx.type != VALUE_INT) {
                    printf("Error: Invalid array assignment target at [Line %d]\n", node->line);
                    return makeNull();
                }
                int i = idx.data.intValue;
                if (i < 0 || i >= arr.data.arrayValue.count) {
                    printf("Error: Index out of bounds at [Line %d]\n", node->line);
                    return makeNull();
                }
                arr.data.arrayValue.elements[i] = val;
            }
            return val;
        }

        case NODE_AUGMENTED_ASSIGN: {
            Value val = evaluate(node->right, env);
            if (val.type == VALUE_RETURN) return val;

            if (node->left->type == NODE_IDENTIFIER) {
                Value current = env_get(env, node->left->name);
                Value newVal = applyBinaryOp(node->name, current, val);
                env_set(env, node->left->name, newVal);
                return newVal;
            } else if (node->left->type == NODE_ARRAY_INDEX) {
                Value arr = evaluate(node->left->left, env);
                Value idx = evaluate(node->left->right, env);
                if (arr.type != VALUE_ARRAY || idx.type != VALUE_INT) {
                    printf("Error: Invalid augmented assignment target at [Line %d]\n", node->line);
                    return makeNull();
                }
                int i = idx.data.intValue;
                if (i < 0 || i >= arr.data.arrayValue.count) {
                    printf("Error: Index out of bounds at [Line %d]\n", node->line);
                    return makeNull();
                }
                Value current = arr.data.arrayValue.elements[i];
                Value newVal = applyBinaryOp(node->name, current, val);
                arr.data.arrayValue.elements[i] = newVal;
                return newVal;
            }
            return makeNull();
        }

        case NODE_VAR_DECL: {
            Value val = makeNull();
            if (node->left) {
                val = evaluate(node->left, env);
                if (val.type == VALUE_RETURN) return val;
            }
            env_define(env, node->name, val);
            return val;
        }

        case NODE_BLOCK: {
            Env* blockEnv = create_environment(env);
            ASTNode* stmt = node->body;
            Value result = makeNull();
            while (stmt != NULL) {
                result = evaluate(stmt, blockEnv);
                if (result.type == VALUE_RETURN) {
                    free_environment(blockEnv);
                    return result;
                }
                stmt = stmt->next;
            }
            free_environment(blockEnv);
            return result;
        }

        case NODE_PRINT: {
            Value val = evaluate(node->left, env);
            if (val.type == VALUE_RETURN) return val;
            printValue(val);
            printf("\n");
            return val;
        }

        case NODE_FUNCTION_DEF: {
            Value fn = makeFunction(node, env);
            env_define(env, node->name, fn);
            return fn;
        }

        case NODE_RETURN: {
            Value val = makeNull();
            if (node->left) {
                val = evaluate(node->left, env);
                if (val.type == VALUE_RETURN) return val;
            }
            return makeReturn(val);
        }

        case NODE_FUNCTION: {
            Value func = env_get(env, node->name);

            if (func.type == VALUE_FUNCTION) {
                ASTNode* decl = func.data.functionValue.declaration;
                Env* closure = func.data.functionValue.closure;
                Env* callEnv = create_environment(closure);

                ASTNode* param = decl->left;
                ASTNode* arg = node->body;

                while (param && arg) {
                    Value val = evaluate(arg, env);
                    if (val.type == VALUE_RETURN) {
                        free_environment(callEnv);
                        return val;
                    }
                    env_define(callEnv, param->name, val);
                    param = param->next;
                    arg = arg->next;
                }

                Value result = evaluate(decl->body, callEnv);
                if (result.type == VALUE_RETURN) {
                    Value unwrapped = *(result.data.returnValue);
                    free(result.data.returnValue);
                    result = unwrapped;
                }

                free_environment(callEnv);
                return result;
            }
            if (func.type == VALUE_NATIVE) {
                // Count arguments
                int argCount = 0;
                ASTNode* arg = node->body;
                while (arg) {
                    argCount++;
                    arg = arg->next;
                }

                // Evaluate arguments
                Value* args = NULL;
                if (argCount > 0) {
                    args = malloc(sizeof(Value) * argCount);
                    arg = node->body;
                    for (int i = 0; i < argCount; i++) {
                        args[i] = evaluate(arg, env);
                        arg = arg->next;
                    }
                }

                Value result = func.data.nativeFn(argCount, args);
                if (args) free(args);
                return result;
            }

            if (node->name == NULL) return makeNull();

            printf("Unknown function: '%s' at [Line %d, Col %d]\n", node->name, node->line, node->column);
            return makeNull();
        }

        case NODE_UNKNOWN: {
            printf("Unknown token: %s\n", node->name);
            return makeNull();
        }

        default:
            printf("Unsupported node type: %d\n", node->type);
            return makeNull();
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