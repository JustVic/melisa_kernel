#include "lib.h"

int main(void)
{
//	printf("process 1 runs!\n");

	int pid;

	pid = fork();
	printf("user1 starts");
	if (pid == 0) {
		//printf("this is new process\n");
		printf("user1 2   ");

		exec("TEST.BIN");
		printf("user1 3  ");

	}
	else {
		//printf("this is the current process\n");
		waitu(pid);
		printf("test process exits");
	}

/*	int fd;
	int size;
	char buffer[100] = { 0 };

	printf("open file testing");
	fd = open_file("TEST.BIN");

	if (fd == -1) {
		printf("open file failed");
	}
	else {
		size = get_file_size(fd);
		size = read_file(fd, buffer, size);
		printf("%s", buffer);
		printf("read %db in total", size);
	}*/

//	while (1) {
	/*	printf("process 1!\n");

		waitu();
		printf("process 1 running! \t");*/
//	}

	return 0;
}
