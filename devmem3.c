#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define PAGE_MASK ((PAGE_SIZE) - 1)

int main(int argc, const char *argv[])
{
	off_t addr;
	ssize_t len;
	bool read_flag;
	int fd;

	if (argc != 4)
		goto usage;

	addr = (off_t)strtoul(argv[1], NULL, 0);
	len = (ssize_t)strtoul(argv[3], NULL, 0);

	if (!strcmp(argv[2], "r"))
		read_flag = true;
	else if (!strcmp(argv[2], "w"))
		read_flag = false;
	else
		goto usage;

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd == -1) {
		fprintf(stderr, "opening /dev/mem failed: %m\n");
		return 1;
	}

	while (len > 0) {
		void *page_start;
		off_t page_offset = addr & PAGE_MASK;
		ssize_t to_rw;
		ssize_t rw;

		if (len > (PAGE_SIZE - page_offset))
			to_rw = PAGE_SIZE - page_offset;
		else
			to_rw = len;

		page_start = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr & ~PAGE_MASK);
		if (page_start == (void *)-1) {
			fprintf(stderr, "mmap failed: %m\n");
			goto close_fd;
		}

		if (read_flag)
			rw = write(STDOUT_FILENO, page_start + page_offset, to_rw);
		else
			rw = read(STDIN_FILENO, page_start + page_offset, to_rw);

		if (rw == -1) {
			fprintf(stderr, "read/write failed: %m\n");
			munmap(page_start, PAGE_SIZE);
			goto close_fd;
		} else if (rw != to_rw) {
			fprintf(stderr, "short read/write\n");
			munmap(page_start, PAGE_SIZE);
			goto close_fd;
		}

		munmap(page_start, PAGE_SIZE);

		addr += to_rw;
		len -= to_rw;
	}

close_fd:
	close(fd);
	return 0;
	
usage:
	fprintf(stderr, "\n");
	fprintf(stderr, "usage: %s address r|w count\n", argv[0]);
	fprintf(stderr, "       address - location in memory to read/write\n");
	fprintf(stderr, "           r|w - is this a read or a write?\n");
	fprintf(stderr, "         count - how many bytes to read/write\n\n");
	fprintf(stderr, "  this command allows reading/writing large buffers of data to/from /dev/mem\n");
	fprintf(stderr, "  read data is piped to stdout, and write data is taken from stdin\n\n");
	return 1;
}
