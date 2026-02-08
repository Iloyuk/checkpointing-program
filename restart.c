#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <ucontext.h>

struct ckpt_segment {
	void *start_address;
	void *end_address;
	int read, write, execute;
	int is_register_context;
	int data_size;
	char name[80];
};

void do_work(char* fname) {
	int fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror("open restore");
		exit(1);
	}

	ucontext_t old_registers;
	struct ckpt_segment segment;
	while (1) {
		ssize_t info = read(fd, &segment, sizeof(struct ckpt_segment));
		if (info == 0) { break; }
		if (segment.is_register_context) { // Read CPU registers
			read(fd, &old_registers, sizeof(ucontext_t));
		} else { // Restore memory segments
			void *addr = mmap(segment.start_address, segment.data_size, 
					PROT_READ | PROT_WRITE | PROT_EXEC,
					MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
			if (addr == MAP_FAILED) { 
				perror("mmap restore");
				exit(1);
		       	}

			int rc = 0;
			while (rc < segment.data_size) {
				ssize_t rc2 = read(fd, 
						(char *)segment.start_address + rc,
						segment.data_size - rc);
				if (rc2 <= 0) {
					perror("read restore");
					exit(1);
				}
				rc += rc2;
			}
		}
	}
	close(fd);
	setcontext(&old_registers);
}

int main(int argc, char* argv[]) {
	void *addr = mmap((void *)0x6000000, 0x1000000, 
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
	if (addr == MAP_FAILED) {
		perror("mmap main");
		exit(1);
	}

	void *stack_ptr = (void *)(0x6000000 + 0x1000000 - 16);
	asm volatile ("mov %0,%%rsp;" : : "g" (stack_ptr) : "memory");
	do_work(argv[1]);

	return 0;
}
