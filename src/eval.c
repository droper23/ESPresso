// eval.c
// Created by Derek Roper on 3/2/26.
//

#include "eval.h"
#include "ast.h"
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "env.h"


static void evalRuntimeErrorAt(int line, int column, const char* format, ...);

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
        result.stringOwnership = STRING_OWNERSHIP_VALUE;
        result.isBorrowed = false;
        return result;
    }

    int useFloat = (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT);
    float l = (left.type  == VALUE_FLOAT) ? left.data.floatValue  : (float)left.data.intValue;
    float r = (right.type == VALUE_FLOAT) ? right.data.floatValue : (float)right.data.intValue;

    if (strcmp(op, "+")  == 0) return useFloat ? makeFloat(l + r) : makeInt((int)(l + r));
    if (strcmp(op, "-")  == 0) return useFloat ? makeFloat(l - r) : makeInt((int)(l - r));
    if (strcmp(op, "*")  == 0) return useFloat ? makeFloat(l * r) : makeInt((int)(l * r));
    if (strcmp(op, "/")  == 0) {
        if (r == 0.0f) { evalRuntimeErrorAt(left.line, left.column, "division by zero"); return makeNull(); }
        return useFloat ? makeFloat(l / r) : makeInt((int)(l / r));
    }
    if (strcmp(op, ">")  == 0) return makeBool(l >  r);
    if (strcmp(op, "<")  == 0) return makeBool(l <  r);
    if (strcmp(op, "==") == 0) return makeBool(l == r);
    if (strcmp(op, "!=") == 0) return makeBool(l != r);
    if (strcmp(op, "<=") == 0) return makeBool(l <= r);
    if (strcmp(op, ">=") == 0) return makeBool(l >= r);

    evalRuntimeErrorAt(left.line, left.column, "unsupported binary operation: '%s'", op);
    return makeNull();
}

static Value evaluate_internal(ASTNode* node, Env* env);

typedef struct {
    const char* name;
    int line;
    int column;
} EvalFrame;

static EvalFrame evalFrames[256];
static int evalFrameCount = 0;
static const char* evalSource = NULL;
static const char* evalSourcePath = NULL;

void evalSetSource(const char* source, const char* path) {
    evalSource = source;
    evalSourcePath = path;
}

static void pushEvalFrame(const char* name, int line, int column) {
    if (evalFrameCount >= 256) return;
    evalFrames[evalFrameCount++] = (EvalFrame){ name ? name : "script", line, column };
}

static void popEvalFrame(void) {
    if (evalFrameCount > 0) evalFrameCount--;
}

static void printEvalSourceLine(int line, int column) {
    if (!evalSource || line <= 0) return;
    const char* p = evalSource;
    const char* lineStart = evalSource;
    int currentLine = 1;
    while (*p && currentLine < line) {
        if (*p == '\n') {
            currentLine++;
            lineStart = p + 1;
        }
        p++;
    }
    const char* lineEnd = lineStart;
    while (*lineEnd && *lineEnd != '\n') lineEnd++;
    fprintf(stderr, "  %.*s\n", (int)(lineEnd - lineStart), lineStart);
    if (column > 0) {
        fprintf(stderr, "  ");
        for (int i = 1; i < column; i++) fputc(' ', stderr);
        fputc('^', stderr);
        fputc('\n', stderr);
    }
}

static void evalRuntimeErrorAt(int line, int column, const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);

    if (column > 0) {
        fprintf(stderr, "[line %d, col %d]\n", line, column);
    } else if (line > 0) {
        fprintf(stderr, "[line %d]\n", line);
    }
    printEvalSourceLine(line, column);

    fprintf(stderr, "Stack trace:\n");
    if (evalFrameCount == 0) {
        if (evalSourcePath) {
            if (column > 0) fprintf(stderr, "  at script (%s:%d:%d)\n", evalSourcePath, line, column);
            else if (line > 0) fprintf(stderr, "  at script (%s:%d)\n", evalSourcePath, line);
            else fprintf(stderr, "  at script\n");
        } else {
            if (column > 0) fprintf(stderr, "  at script (line %d, col %d)\n", line, column);
            else if (line > 0) fprintf(stderr, "  at script (line %d)\n", line);
            else fprintf(stderr, "  at script\n");
        }
        return;
    }
    for (int i = evalFrameCount - 1; i >= 0; i--) {
        EvalFrame* f = &evalFrames[i];
        if (evalSourcePath) {
            if (f->column > 0) {
                fprintf(stderr, "  at %s (%s:%d:%d)\n", f->name, evalSourcePath, f->line, f->column);
            } else {
                fprintf(stderr, "  at %s (%s:%d)\n", f->name, evalSourcePath, f->line);
            }
        } else {
            if (f->column > 0) {
                fprintf(stderr, "  at %s (line %d, col %d)\n", f->name, f->line, f->column);
            } else {
                fprintf(stderr, "  at %s (line %d)\n", f->name, f->line);
            }
        }
    }
}

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

            int truthy = isTruthy(cond);
            freeValueContents(cond);

            if (truthy) {
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
                int truthy = isTruthy(cond);
                freeValueContents(cond);
                if (!truthy) break;

                result = evaluate(node->body, env);
                if (result.type == VALUE_RETURN) return result;
                if (result.type == VALUE_BREAK) {
                    freeValueContents(result);
                    result = makeNull();
                    break;
                }
                if (result.type == VALUE_CONTINUE) {
                    freeValueContents(result);
                    continue;
                }
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
                    Value elemVal = evaluate(elemNode, env);
                    elements[i] = valueMakeOwned(elemVal);
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
                evalRuntimeErrorAt(node->line, node->column, "indexing requires an array, found %d", left.type);
                freeValueContents(left);
                freeValueContents(right);
                return makeNull();
            }
            if (right.type != VALUE_INT) {
                evalRuntimeErrorAt(node->line, node->column, "array index must be an integer");
                freeValueContents(left);
                freeValueContents(right);
                return makeNull();
            }

            int idx = right.data.intValue;
            if (idx < 0 || idx >= left.data.arrayValue.count) {
                evalRuntimeErrorAt(node->line, node->column, "array index out of bounds: %d", idx);
                freeValueContents(left);
                freeValueContents(right);
                return makeNull();
            }
            Value elem = left.data.arrayValue.elements[idx];
            Value result = elem;
            if (left.isBorrowed) {
                result.isBorrowed = true;
            } else {
                result = valueMakeOwned(elem);
            }

            freeValueContents(right);
            if (!left.isBorrowed) freeValueContents(left);
            return result;
        }

        case NODE_RANGE: {
            Value left = evaluate(node->left, env);
            if (left.type == VALUE_RETURN) return left;
            Value right = evaluate(node->right, env);
            if (right.type == VALUE_RETURN) return right;

            if (left.type != VALUE_INT || right.type != VALUE_INT) {
                evalRuntimeErrorAt(node->line, node->column, "range bounds must be integers");
                freeValueContents(left);
                freeValueContents(right);
                return makeNull();
            }
            Value range = makeRange(left.data.intValue, right.data.intValue);
            freeValueContents(left);
            freeValueContents(right);
            return range;
        }

        case NODE_FOR: {
            Value rangeVal = evaluate(node->left, env);
            if (rangeVal.type == VALUE_RETURN) return rangeVal;

            if (rangeVal.type != VALUE_RANGE) {
                evalRuntimeErrorAt(node->line, node->column, "for loop expects a range");
                freeValueContents(rangeVal);
                return makeNull();
            }

            int start = rangeVal.data.rangeValue.start;
            int end = rangeVal.data.rangeValue.end;
            freeValueContents(rangeVal);

            Env* forEnv = create_environment(env);
            Value result = makeNull();

            for (int i = start; i < end; i++) {
                env_define(forEnv, node->loopVar, makeInt(i));
                result = evaluate(node->body, forEnv);
                if (result.type == VALUE_RETURN) {
                    free_environment(forEnv);
                    return result;
                }
                if (result.type == VALUE_BREAK) {
                    freeValueContents(result);
                    result = makeNull();
                    break;
                }
                if (result.type == VALUE_CONTINUE) {
                    freeValueContents(result);
                    continue;
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
                freeValueContents(left);
                return evaluate(node->right, env);
            }
            if (strcmp(node->name, "||") == 0 || strcmp(node->name, "or") == 0) {
                Value left = evaluate(node->left, env);
                if (left.type == VALUE_RETURN) return left;
                if (isTruthy(left)) return left;
                freeValueContents(left);
                return evaluate(node->right, env);
            }

            if (strcmp(node->name, "!") == 0) {
                Value right = evaluate(node->right, env);
                if (right.type == VALUE_RETURN) return right;
                Value result = makeBool(!isTruthy(right));
                freeValueContents(right);
                return result;
            }
            if (strcmp(node->name, "-") == 0 && node->left == NULL) {
                Value right = evaluate(node->right, env);
                if (right.type == VALUE_RETURN) return right;
                Value result = (right.type == VALUE_FLOAT)
                                   ? makeFloat(-right.data.floatValue)
                                   : makeInt(-right.data.intValue);
                freeValueContents(right);
                return result;
            }

            Value left  = evaluate(node->left,  env);
            if (left.type == VALUE_RETURN) return left;
            Value right = evaluate(node->right, env);
            if (right.type == VALUE_RETURN) return right;

            if (node->name == NULL) {
                evalRuntimeErrorAt(node->line, node->column, "unsupported binary operation");
                freeValueContents(left);
                freeValueContents(right);
                return makeNull();
            }

            Value result = applyBinaryOp(node->name, left, right);
            freeValueContents(left);
            freeValueContents(right);
            return result;
        }

        case NODE_ASSIGN: {
            Value val = evaluate(node->right, env);
            if (val.type == VALUE_RETURN) return val;
            
            if (node->left->type == NODE_IDENTIFIER) {
                if (!env_set(env, node->left->name, val)) {
                    evalRuntimeErrorAt(node->line, node->column, "undefined variable '%s'",
                                       node->left->name ? node->left->name : "");
                }
                return env_get(env, node->left->name);
            } else if (node->left->type == NODE_ARRAY_INDEX) {
                Value arr = evaluate(node->left->left, env);
                Value idx = evaluate(node->left->right, env);
                if (arr.type != VALUE_ARRAY || idx.type != VALUE_INT) {
                    evalRuntimeErrorAt(node->line, node->column, "invalid array assignment target");
                    freeValueContents(arr);
                    freeValueContents(idx);
                    freeValueContents(val);
                    return makeNull();
                }
                int i = idx.data.intValue;
                if (i < 0 || i >= arr.data.arrayValue.count) {
                    evalRuntimeErrorAt(node->line, node->column, "index out of bounds");
                    freeValueContents(arr);
                    freeValueContents(idx);
                    freeValueContents(val);
                    return makeNull();
                }
                freeValueContents(arr.data.arrayValue.elements[i]);
                arr.data.arrayValue.elements[i] = valueMakeOwned(val);
                Value result = arr.data.arrayValue.elements[i];
                if (arr.isBorrowed) {
                    result.isBorrowed = true;
                } else {
                    result = valueMakeOwned(result);
                    freeValueContents(arr);
                }
                freeValueContents(idx);
                return result;
            }
            return makeNull();
        }

        case NODE_AUGMENTED_ASSIGN: {
            Value val = evaluate(node->right, env);
            if (val.type == VALUE_RETURN) return val;

            if (node->left->type == NODE_IDENTIFIER) {
                Value current = env_get(env, node->left->name);
                Value newVal = applyBinaryOp(node->name, current, val);
                if (!env_set(env, node->left->name, newVal)) {
                    evalRuntimeErrorAt(node->line, node->column, "undefined variable '%s'",
                                       node->left->name ? node->left->name : "");
                }
                return env_get(env, node->left->name);
            } else if (node->left->type == NODE_ARRAY_INDEX) {
                Value arr = evaluate(node->left->left, env);
                Value idx = evaluate(node->left->right, env);
                if (arr.type != VALUE_ARRAY || idx.type != VALUE_INT) {
                    evalRuntimeErrorAt(node->line, node->column, "invalid augmented assignment target");
                    freeValueContents(arr);
                    freeValueContents(idx);
                    freeValueContents(val);
                    return makeNull();
                }
                int i = idx.data.intValue;
                if (i < 0 || i >= arr.data.arrayValue.count) {
                    evalRuntimeErrorAt(node->line, node->column, "index out of bounds");
                    freeValueContents(arr);
                    freeValueContents(idx);
                    freeValueContents(val);
                    return makeNull();
                }
                Value current = arr.data.arrayValue.elements[i];
                Value newVal = applyBinaryOp(node->name, current, val);
                freeValueContents(arr.data.arrayValue.elements[i]);
                arr.data.arrayValue.elements[i] = valueMakeOwned(newVal);
                Value result = arr.data.arrayValue.elements[i];
                if (arr.isBorrowed) {
                    result.isBorrowed = true;
                } else {
                    result = valueMakeOwned(result);
                    freeValueContents(arr);
                }
                freeValueContents(idx);
                return result;
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
            return env_get(env, node->name);
        }

        case NODE_BLOCK: {
            Env* blockEnv = create_environment(env);
            ASTNode* stmt = node->body;
            Value result = makeNull();
            while (stmt != NULL) {
                freeValueContents(result);
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
            freeValueContents(val);
            return makeNull();
        }

        case NODE_FUNCTION_DEF: {
            Value fn = makeFunction(node, env);
            env_define(env, node->name, fn);
            return env_get(env, node->name);
        }

        case NODE_RETURN: {
            Value val = makeNull();
            if (node->left) {
                val = evaluate(node->left, env);
                if (val.type == VALUE_RETURN) return val;
            }
            return makeReturn(val);
        }
        case NODE_BREAK:
            return makeBreak();
        case NODE_CONTINUE:
            return makeContinue();

        case NODE_FUNCTION: {
            Value func = evaluate(node->left, env);
            if (func.type == VALUE_RETURN) return func;

            if (func.type == VALUE_FUNCTION) {
                ASTNode* decl = func.data.functionValue.declaration;
                Env* closure = func.data.functionValue.closure;
                Env* callEnv = create_environment(closure);

                pushEvalFrame(decl->name ? decl->name : "fn", node->line, node->column);

                ASTNode* param = decl->left;
                ASTNode* arg = node->body;

                while (param && arg) {
                    Value val = evaluate(arg, env);
                    if (val.type == VALUE_RETURN) {
                        popEvalFrame();
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
                freeValueContents(func);
                popEvalFrame();
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
                if (args) {
                    for (int i = 0; i < argCount; i++) {
                        freeValueContents(args[i]);
                    }
                    free(args);
                }
                freeValueContents(func);
                return result;
            }

            freeValueContents(func);
            evalRuntimeErrorAt(node->line, node->column, "unknown function");
            return makeNull();
        }

        case NODE_UNKNOWN: {
            evalRuntimeErrorAt(node->line, node->column, "unknown token: %s",
                               node->name ? node->name : "");
            return makeNull();
        }

        default:
            evalRuntimeErrorAt(node->line, node->column, "unsupported node type: %d", node->type);
            return makeNull();
    }
}

void freeAST(ASTNode* node) {
    if (!node) return;

    freeAST(node->left);
    freeAST(node->right);
    freeAST(node->body);
    freeAST(node->alternate);
    freeAST(node->increment);

    if (node->loopVar) free(node->loopVar);
    if (node->name) free(node->name);

    ASTNode* next = node->next;
    free(node);
    freeAST(next);
}
