// env.c
// Created by Derek Roper on 3/2/26.
//

#include "env.h"
#include <stdlib.h>
#include <string.h>

Env* create_environment(Env* parent) {
    Env* env = malloc(sizeof(Env));
    env->count = 0;
    env->parent = parent;
    for (int i = 0; i < MAX_VARS; i++) {
        env->variables[i].name = NULL;
        env->variables[i].value = 0;
    }
    return env;
}

int env_get(Env* env, const char* name) {
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->variables[i].name, name) == 0) {
            return env->variables[i].value;
        }
    }
    if (env->parent != NULL) {
        return env_get(env->parent, name);
    } else {
        return 0;
    }
}

void env_set(Env* env, const char* name, int value) {
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
    }
}

void free_environment(Env* env) {
    for (int i = 0; i < env->count; i++) {
        free(env->variables[i].name);
    }
    free(env);
}