#include <stdio.h>

#include "include/sds.h"

int main(int argc, char const* argv[]) {
    sds s = sdsnew("Hello World!");
    printf("%s\n", s);
    return 0;
}
