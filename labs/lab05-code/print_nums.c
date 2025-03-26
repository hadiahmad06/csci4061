// Author: John Kolb <jhkolb@umn.edu>
// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <unistd.h>

int main() {
    int i = 0;
    while (1) {
        printf("%d\n", i);
        i++;
        sleep(1);
    }

    return 0;
}
