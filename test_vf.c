#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	char temp1, temp2;
	int fd, i;

	printf("opening the file /dev/vf-driver \n");
	fd = open("/dev/vf-driver", O_RDWR);
	if (fd < 0) {
		printf("open /dev/vf-driver failed\n");
		exit (-1);
	} else printf("open /dev/vf-driver successfully\n");
	temp1 = '1';
	i = write(fd, temp1, 1);
	printf("number of char write = %d\n", i);

	i = read(fd, temp2, 1);
	printf("number of char read = %d\n", i);

	if (i < 0) {
		printf("read error\n");
		close(fd);
		exit (-1);
	} else printf ("value read = %c\n", temp2);
	close(fd);
	exit (0);
}
