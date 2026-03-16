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
    return var ? var->value : makeNull();
}

void env_set(Env* env, const char* name, Value value) {
    struct Variable* var = find_variable(env, name);

    if (var != NULL) {
        var->value = value;
        return;
    }

    printf("Error: undefined variable '%s'\n", name);
}

void env_define(Env* env, const char* name, Value value) {
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->variables[i].name, name) == 0) {
            env->variables[i].value = value;
            return;
        }
    }

    if (env->count < MAX_VARS) {
        env->variables[env->count].name = strdup(name);
        env->variables[env->count].value = value;
        env->count++;
    } else {
        printf("Error: environment is full, cannot define '%s'\n", name);
    }
}

void free_environment(Env* env) {
    env_unref(env);
}