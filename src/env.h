// env.h
// Created by Derek Roper on 3/2/26.
//

#ifndef ENV_H
#define ENV_H

#include <stddef.h>
#include "value.h"

#define MAX_VARS 64

struct Variable {
    char* name;
    Value value;
};

typedef struct Env {
    struct Variable variables[MAX_VARS];
    int count;
    struct Env* parent;
    int ref_count;
} Env;

Env* create_environment(Env* parent);

Value env_get(Env* env, const char* name);

void env_set(Env* env, const char* name, Value value);

void env_define(Env* env, const char* name, Value value);

void free_environment(Env* env);
void env_ref(Env* env);
void env_unref(Env* env);

#endif //ENV_H