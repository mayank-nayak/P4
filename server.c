#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "udp.h"
#include "mfs.h"
#include "ufs.h"


int main(int argc, char *argv[]) {
	//struct sockaddr_in addrSnd, addrRcv;

	// get file system image
	int fd = open(argv[1], O_RDWR);
	// if file system image doesn't exist, then exit
	if (fd < 0) {
		perror("image does not exist\n");
		exit(1);
	}
	
	// get super block
	super_t s;
	read(fd, &s, sizeof(super_t));




	
	close(fd);


}