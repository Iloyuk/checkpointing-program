#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <assert.h>
#include <ucontext.h>
#include <asm/prctl.h>
#include <sys/prctl.h>

#define NAME_LEN 80

typedef void (*printf_ptr_t)();
typedef void (*fnc_ptr_p)(const char*, void *, printf_ptr_t);

struct proc_maps_line {
	void *start;
	void *end;
	char rwxp[4];
	char name[NAME_LEN];
};

struct ckpt_segment {
	void *start_address;
	void *end_address;
	int read, write, execute;
	int is_register_context;
	int data_size;
	char name[80];
};

static struct proc_maps_line proc_maps[1000];
static struct ckpt_segment ckpt_segments[1000];

int match_one_line(int proc_maps_fd, struct proc_maps_line *proc_maps_line, char *filename) {
	unsigned long int start, end;
	char rwxp[4];
	char tmp[10];
	int tmp_stdin = dup(0);
	dup2(proc_maps_fd, 0);
	int rc = scanf("%lx-%lx %4c %*s %*s %*[0-9 ]%[^\n]\n", &start, &end, rwxp, filename);
	assert(fseek(stdin, 0, SEEK_CUR) == 0);
	clearerr(stdin);
	dup2(tmp_stdin, 0);
	close(tmp_stdin);
	if (rc == EOF || rc == 0) {
		proc_maps_line -> start = NULL;
		proc_maps_line -> end = NULL;
		return EOF;
	}
	if (rc == 3) {
		strncpy(proc_maps_line -> name, "ANONYMOUS_SEGMENT", strlen("ANONYMOUS_SEGMENT") + 1);
	} else {
		assert(rc == 4);
		strncpy(proc_maps_line -> name, filename, NAME_LEN - 1);
		proc_maps_line -> name[NAME_LEN - 1] = '\0';
	}
	proc_maps_line -> start = (void *)start;
	proc_maps_line -> end = (void *)end;
	memcpy(proc_maps_line -> rwxp, rwxp, 4);
	return 0;
}

int proc_self_maps(struct proc_maps_line proc_maps[]) {
	int proc_maps_fd = open("/proc/self/maps", O_RDONLY);
	char filename[100];
	int rc = -2;
	for (int i = 0; rc != EOF; i++) {
		rc = match_one_line(proc_maps_fd, &proc_maps[i], filename);
	}
	close(proc_maps_fd);
	return 0;
}

int write_helper(int fd, void *buf, size_t count) {
	ssize_t rc = write(fd, buf, count);
	char *ptr = (char *)buf;

	if (rc == -1) {
		return -1;
	}
	while (rc < count) {
		ssize_t rc2 = write(fd, ptr + rc, count - rc);
		if (rc2 == -1) {
			return -1;
		}
		rc += rc2;
	}
	return 0;
}

static int is_restart = 0;
void signal_handler(int sig) {
	// Save memory segments
	assert(proc_self_maps(proc_maps) == 0);
	int sgmt_cnt = 0;
	for (int i = 0; proc_maps[i].start != NULL; i++) {
		if (strcmp(proc_maps[i].name, "[vdso]") != 0
			&& strcmp(proc_maps[i].name, "[vvar]") != 0
			&& strcmp(proc_maps[i].name, "[vsyscall]") != 0) {
			if (proc_maps[i].rwxp[0] == 'r') {
	                        ckpt_segments[sgmt_cnt].start_address = proc_maps[i].start;
        	                ckpt_segments[sgmt_cnt].end_address = proc_maps[i].end;
                	        ckpt_segments[sgmt_cnt].read = (proc_maps[i].rwxp[0] == 'r');
	                        ckpt_segments[sgmt_cnt].write = (proc_maps[i].rwxp[1] == 'w');
        	                ckpt_segments[sgmt_cnt].execute = (proc_maps[i].rwxp[2] == 'x');
                	        ckpt_segments[sgmt_cnt].is_register_context = 0;
	                        ckpt_segments[sgmt_cnt].data_size = (char*)proc_maps[i].end - (char*)proc_maps[i].start;
        	                strcpy(ckpt_segments[sgmt_cnt].name, proc_maps[i].name);
                	        sgmt_cnt++;
                	}
		}
	}

	// Save registers and file descriptor register
	unsigned long saved_fs;
	ucontext_t context;
	arch_prctl(ARCH_GET_FS, &saved_fs);
        getcontext(&context);
	arch_prctl(ARCH_SET_FS, saved_fs);
	if (is_restart != 0) { // Back after a restart
		pid_t current_pid = getpid();
		printf("RESTARTED IN ORIGINAL PROGRAM - pid %d\n", current_pid);
		fflush(stdout);
		is_restart = 0;
		signal(SIGUSR2, signal_handler);
		return; 
	}

	is_restart = 1;
	write(1, "WRITING CKPT\n", 14);
	ckpt_segments[sgmt_cnt].start_address = &context;
	ckpt_segments[sgmt_cnt].end_address = (char *)&context + sizeof(ucontext_t);
	ckpt_segments[sgmt_cnt].is_register_context = 1;
	ckpt_segments[sgmt_cnt].data_size = sizeof(ucontext_t);
	sgmt_cnt++;
	
	// Save all data to file 
	int fd = open("myckpt.img", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd == -1) { _exit(1); }
	int sgmt_len = sizeof(struct ckpt_segment);
	for (int i = 0; i < sgmt_cnt; i++) {
		if (write_helper(fd, &ckpt_segments[i], sgmt_len) == -1) { // Header
			_exit(1);
		}
		if (write_helper(fd, ckpt_segments[i].start_address, ckpt_segments[i].data_size) == -1) { // Data
			_exit(1);
		}
	}

	close(fd);
	is_restart = 0;
	exit(0);
}

void __attribute__((constructor))
my_constructor() {
	signal(SIGUSR2, signal_handler);
}
