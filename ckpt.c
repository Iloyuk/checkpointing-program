#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	setenv("LD_PRELOAD", "./libckpt.so", 1);
	pid_t pid = getpid();
	printf("Current running process id is %d\n", pid);
	execvp(argv[1], &argv[1]);
	return 0;
}
