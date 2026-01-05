#include <stdio.h>
#include <unistd.h>

void do_work(char* filename) {
	printf("Real user id: %u.\nEffective user id: %u.\n",
    	getuid(), geteuid());

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("File could not be opened.\n");
    } else {
        fclose(file);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2){
        printf("filename argument is missing\n");
        return -1;
    }

    do_work(argv[1]);
    setuid(geteuid());
    do_work(argv[1]);

    return 0;
}
