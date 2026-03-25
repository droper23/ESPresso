// env.c
// Created by Derek Roper on 3/2/26.
//

#include "env.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_value_contents_force(Value v) {
    if (v.isBorrowed) return;
    if (v.type == VALUE_STRING && v.stringOwnership == STRING_OWNERSHIP_VALUE) {
        free((void*)v.data.stringValue);
        return;
    }
    if (v.type == VALUE_RETURN) {
        if (v.data.returnValue) {
            free_value_contents_force(*(v.data.returnValue));
            free(v.data.returnValue);
        }
        return;
    }
    if (v.type == VALUE_ARRAY) {
        for (int i = 0; i < v.data.arrayValue.count; i++) {
            free_value_contents_force(v.data.arrayValue.elements[i]);
        }
        free(v.data.arrayValue.elements);
        return;
    }
    if (v.type == VALUE_FUNCTION) {
        return;
    }
}

typedef struct EnvVisited {
    Env* env;
    struct EnvVisited* next;
} EnvVisited;

static bool env_already_freed(Env* env) {
    static EnvVisited* visited = NULL;
    EnvVisited* node = visited;
    while (node) {
        if (node->env == env) return true;
        node = node->next;
    }
    EnvVisited* entry = malloc(sizeof(EnvVisited));
    entry->env = env;
    entry->next = visited;
    visited = entry;
    return false;
}

static struct Variable* find_variable(Env* env, const char* name) {
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->variables[i].name, name) == 0) {
            return &env->variables[i];
        }
    }
    if (env->parent != NULL) {
        return find_variable(env->parent, name);
    }
    return NULL;
}

Env* create_environment(Env* parent) {
    Env* env = malloc(sizeof(Env));
    env->count = 0;
    env->parent = parent;
    env->ref_count = 1;
    if (parent) parent->ref_count++;

    for (int i = 0; i < MAX_VARS; i++) {
        env->variables[i].name = NULL;
        env->variables[i].value = makeNull();
    }
    return env;
}

void env_ref(Env* env) {
    if (env) env->ref_count++;
}

void env_unref(Env* env) {
    if (!env) return;
    env->ref_count--;
    if (env->ref_count == 0) {
        env->ref_count = -1;
        Env* parent = env->parent;
        for (int i = 0; i < env->count; i++) {
            if (env->variables[i].name) free(env->variables[i].name);
            freeValueContents(env->variables[i].value);
        }
        free(env);
        if (parent) env_unref(parent);
    }
}

Value env_get(Env* env, const char* name) {
    struct Variable* var = find_variable(env, name);
    if (!var) return makeNull();
    Value v = var->value;
    v.isBorrowed = true;
    return v;
}

bool env_set(Env* env, const char* name, Value value) {
    struct Variable* var = find_variable(env, name);

    if (var != NULL) {
        Value owned = valueMakeOwned(value);
        freeValueContents(var->value);
        var->value = owned;
        return true;
    }

    return false;
}

void env_define(Env* env, const char* name, Value value) {
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->variables[i].name, name) == 0) {
            Value owned = valueMakeOwned(value);
            freeValueContents(env->variables[i].value);
            env->variables[i].value = owned;
            return;
        }
    }

    if (env->count < MAX_VARS) {
        env->variables[env->count].name = strdup(name);
        env->variables[env->count].value = valueMakeOwned(value);
        env->count++;
    } else {
        fprintf(stderr, "Error: environment is full, cannot define '%s'\n", name);
    }
}

void free_environment(Env* env) {
    env_unref(env);
}

void free_environment_force(Env* env) {
    if (!env) return;
    if (env_already_freed(env)) return;

    Env* parent = env->parent;

    for (int i = 0; i < env->count; i++) {
        Value v = env->variables[i].value;
        if (v.type == VALUE_FUNCTION && v.data.functionValue.closure) {
            free_environment_force(v.data.functionValue.closure);
        }
        if (env->variables[i].name) free(env->variables[i].name);
    }

    for (int i = 0; i < env->count; i++) {
        free_value_contents_force(env->variables[i].value);
    }

    free(env);
    if (parent) env_unref(parent);
}
