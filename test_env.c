#include <stdio.h>
#include "env.h"

// int main(void) {
//     Env* global = create_environment(NULL);
//     env_set(global, "x", 1);
//
//     Env* child = create_environment(global);
//     env_set(child, "x", 2);
//
//     printf("child x = %d\n", env_get(child, "x"));   // 2
//     printf("global x = %d\n", env_get(global, "x")); // 1
//
//     free_environment(child);
//     free_environment(global);
// }