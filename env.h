// env.h
// Created by Derek Roper on 3/2/26.
//

#ifndef ENV_H
#define ENV_H

#include <stddef.h>

#define MAX_VARS 64

struct Variable {
    char* name;
    int value;
};

typedef struct Env {
    struct Variable variables[MAX_VARS];
    int count;
    struct Env* parent;
} Env;

Env* create_environment(Env* parent);

int env_get(Env* env, const char* name);

void env_set(Env* env, const char* name, int value);

void free_environment(Env* env);

static struct Variable* find_variable(Env* env, const char* name);

#endif //ENV_H