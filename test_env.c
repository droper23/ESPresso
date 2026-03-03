#include <stdio.h>
#include "env.h"

int main(void) {
    // Create global environment
    Env* global = create_environment(NULL);
    env_set(global, "x", 10);

    // Create child environment
    Env* child = create_environment(global);

    printf("---- Parent Lookup Test ----\n");
    printf("child sees x = %d (should be 10)\n", env_get(child, "x"));

    printf("\n---- Override Test ----\n");
    env_set(child, "x", 99);
    printf("child x = %d (should be 99)\n", env_get(child, "x"));
    printf("global x = %d (should still be 10)\n", env_get(global, "x"));

    printf("\n---- Independent Variables ----\n");
    env_set(child, "y", 42);
    printf("child y = %d (should be 42)\n", env_get(child, "y"));
    printf("global y = %d (should be 0 or not found)\n", env_get(global, "y"));

    free_environment(child);
    free_environment(global);

    return 0;
}