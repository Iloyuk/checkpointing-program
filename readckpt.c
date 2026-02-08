#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

struct ckpt_segment {
    void *start_address;
    void *end_address;
    int read, write, execute;
    int is_register_context;
    int data_size;
    char name[80];
};

int main(int argc, char *argv[]) {
	char *filename = "myckpt.img";
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return 1;
	}
	
	struct ckpt_segment segment;
	while (1) {
		ssize_t info = read(fd, &segment, sizeof(struct ckpt_segment));
		if (info == 0) { break; }
		if (!segment.is_register_context) {
			printf("start address: %p\nend address: %p\nfilename: %s\n", segment.start_address, segment.end_address, segment.name);
			printf("\n");
		}
		lseek(fd, segment.data_size, SEEK_CUR);
	}
	return 0;
}
