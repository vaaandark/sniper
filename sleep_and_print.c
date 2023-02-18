#include <stdio.h>
#include <unistd.h>
int main(void) {
    int i;
    for (i = 0; i < 100; ++i) {
        printf("%d\n", i);
        fflush(stdout);
        sleep(1);
    }
    return 0;
}
