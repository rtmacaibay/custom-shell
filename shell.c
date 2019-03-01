#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "history.h"
#include "timer.h"

int main(void) {

    double start = get_time();
    print_history();
    sleep(1);
    double end = get_time();

    printf("Time elapsed: %fs\n", end - start);

    return 0;
}
