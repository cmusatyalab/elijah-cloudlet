#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define KVM_MAGIC_OFFSET (4096)  // offset into the start of physical memory in memory image
#define LIBVIRT_MAGIC_OFFSET (4096)  // offset into the start of physical memory in memory image

extern int scan(void *mem, unsigned long pglist_pos, unsigned long page_zero_pos,
		unsigned char *bitmap, unsigned long len);

// Does not handle errors
static inline unsigned long hextol(const char *str)
{
	long hex = 0;

	sscanf(str, " %lx ", &hex);

	return hex;
}

void usage(void)
{
	fprintf(stderr, "Usage: free_page_scan file pglist page_zero size\n");
	fprintf(stderr, "\tfile: VM memory image file\n");
	fprintf(stderr, "\taddress: address of struct pglist_data in the image in hex, in virtual address (>= 0xc0000000)\n");
	fprintf(stderr, "\taddress: address of struct page for pfn 0 in the image in hex, in virtual address (>= 0xc0000000)\n");
	fprintf(stderr, "\tsize: memory size in GB\n");
}

int main(int argc, char *argv[])
{
	int ret;
	int fd;
	void *mem;
	unsigned long pglist_pos;
	unsigned long page_zero_pos;
	size_t file_size;
	off_t seek_pos;
	const char *mem_file_path;
	unsigned long len;
	int i;
	int count;
	size_t mem_size;
	unsigned char *bitmap;

	if (argc != 5) {
		usage();
		return -1;
	}

	mem_file_path = argv[1];

	pglist_pos = hextol(argv[2]);
	page_zero_pos = hextol(argv[3]);
	mem_size = (unsigned long) (atol(argv[4]) * (1 << 20));

	len = mem_size / PAGE_SIZE;

	fd = open(mem_file_path, O_RDONLY);  // Note 4G limit when compiled in 32-bit mode

	seek_pos = lseek(fd, 0, SEEK_END);
	if (seek_pos < -1) {
		fprintf(stderr, "lseek() failed.\n");
		goto lseek_out;
	}

	file_size = (size_t) seek_pos;

	seek_pos = lseek(fd, 0, SEEK_SET);
	if (seek_pos < -1) {
		fprintf(stderr, "lseek() failed.\n");
		goto lseek_out;
	}

	if (file_size < mem_size) {
		fprintf(stderr, "memory image file is too small.\n");
		goto file_size_out;
	}

	mem = mmap(NULL, mem_size, PROT_READ, MAP_PRIVATE, fd, KVM_MAGIC_OFFSET+LIBVIRT_MAGIC_OFFSET);
	if (mem < 0)  {
		fprintf(stderr, "mmap() failed.\n");
		goto mmap_out;
	}

	bitmap = (unsigned char *) malloc(len);

	if (!bitmap) {
		fprintf(stderr, "malloc() failed.\n");
		goto malloc_out;
	}

	ret = scan(mem, pglist_pos, page_zero_pos, bitmap, len);

	if (!ret) {
		count = 0;
		for (i = 0; i < len; i++) {
			if (bitmap[i]) {
				count++;
				fprintf(stdout, "%d\n", i);
			}
		}

		// printf("free memory: %d\n", count * 4096);
	} else {
		if (ret == -1)
			fprintf(stderr, "scan failed.\n");
		else if (ret == -2)
			fprintf(stderr, "invalid nr_zone (possibly window memory).\n");
	}

	free(bitmap);
	
malloc_out:
	munmap(mem, mem_size);
mmap_out:
file_size_out:
lseek_out:
	close(fd);

	return 0;
}
