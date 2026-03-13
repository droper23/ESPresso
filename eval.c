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
        case VALUE_RETURN: return isTruthy(*(v.data.returnValue));
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

        case NODE_NIL: {
            return makeNil();
        }

        case NODE_IF: {
            Value cond = evaluate(node->left, env);
            if (cond.type == VALUE_RETURN) return cond;

            if (isTruthy(cond)) {
                return evaluate(node->body, env);
            } else if (node->alternate) {
                return evaluate(node->alternate, env);
            }
            return makeNil();
        }

        case NODE_WHILE: {
            Value result = makeNil();
            while (1) {
                Value cond = evaluate(node->left, env);
                if (cond.type == VALUE_RETURN) return cond;
                if (!isTruthy(cond)) break;

                result = evaluate(node->body, env);
                if (result.type == VALUE_RETURN) return result;
            }
            return result;
        }

        case NODE_FOR: {
            Env* forEnv = create_environment(env);
            if (node->left) {
                Value init = evaluate(node->left, forEnv);
                if (init.type == VALUE_RETURN) {
                    free_environment(forEnv);
                    return init;
                }
            }

            Value result = makeNil();
            while (1) {
                if (node->right) {
                    Value cond = evaluate(node->right, forEnv);
                    if (cond.type == VALUE_RETURN) {
                        free_environment(forEnv);
                        return cond;
                    }
                    if (!isTruthy(cond)) break;
                }

                result = evaluate(node->body, forEnv);
                if (result.type == VALUE_RETURN) {
                    free_environment(forEnv);
                    return result;
                }

                if (node->increment) { // increment
                    Value inc = evaluate(node->increment, forEnv);
                    if (inc.type == VALUE_RETURN) {
                        free_environment(forEnv);
                        return inc;
                    }
                }
            }
            free_environment(forEnv);
            return result;
        }

        case NODE_BINARY_OP: {
            if (strcmp(node->name, "&&") == 0) {
                Value left = evaluate(node->left, env);
                if (left.type == VALUE_RETURN) return left;
                if (!isTruthy(left)) return left;
                return evaluate(node->right, env);
            }
            if (strcmp(node->name, "||") == 0) {
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
                return makeNil();
            }

            return applyBinaryOp(node->name, left, right);
        }

        case NODE_ASSIGN: {
            Value val = evaluate(node->right, env);
            if (val.type == VALUE_RETURN) return val;
            env_set(env, node->left->name, val);
            return val;
        }

        case NODE_VAR_DECL: {
            Value val = makeNil();
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
            Value result = makeNil();
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
            Value val = makeNil();
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
                    if (lastVal.type == VALUE_RETURN) return lastVal;
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