// env.c
// Created by Derek Roper on 3/2/26.
//

#include "env.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    for (int i = 0; i < MAX_VARS; i++) {
        env->variables[i].name = NULL;
        env->variables[i].value = makeNil();
    }
    return env;
}

Value env_get(Env* env, const char* name) {
    struct Variable* var = find_variable(env, name);
    return var ? var->value : makeNil();
}

void env_set(Env* env, const char* name, Value value) {
    struct Variable* var = find_variable(env, name);

    if (var != NULL) {
        var->value = value;
        return;
    }

    if (env->count < MAX_VARS) {
        env->variables[env->count].name = strdup(name);
        env->variables[env->count].value = value;
        env->count++;
    } else {
        printf("Error: environment is full, cannot add '%s'\n", name);
    }
}

void free_environment(Env* env) {
    for (int i = 0; i < env->count; i++) {
        free(env->variables[i].name);
    }
    free(env);
}