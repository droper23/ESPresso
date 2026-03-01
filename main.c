#include <stdio.h>

int main(void) {
    const char* source = "print 5 + 3";
    const char* current = source;
    while (*current != '\0') {
        printf("%c\n", *current);
        current++;
    }
}