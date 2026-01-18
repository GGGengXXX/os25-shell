#include <lib.h>
#include <args.h>

void usage(void) {
	debugf("usage: pwd\n");
	exit(1);
}

int main(int argc, char **argv) {

    if (argc != 1) {
        // usage();
        return 1;
        exit(2);
    } else {
        char path[128] = {0};
        getcwd(path);
        printf("%s", path);
    }
    printf("\n");
    exit(0);
    return 0;
}