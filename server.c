#include <stdio.h>
#include "udp.h"

int main(int argc, char *argv[]) {
	struct sockaddr_in addrSnd, addrRcv;

	// get file system image
	int fd = open(argv[1], O_RDWR);
	// if file system image doesn't exist, then exit
	if (fd < 0) {
		perror("image does not exist\n");
		exit(1)
	}

	

}