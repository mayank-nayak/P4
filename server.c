#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "udp.h"
#include "mfs.h"
#include "ufs.h"

#define BUFFER_SIZE (1000)

int main(int argc, char *argv[]) {
	
	
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

	    struct sockaddr_in addrSnd, addrRcv;

	// read in inode bitmap
	void *i_bitMap[UFS_BLOCK_SIZE * s.inode_bitmap_len];
	pread(fd, i_bitMap, s.inode_bitmap_len * UFS_BLOCK_SIZE, UFS_BLOCK_SIZE * s.inode_bitmap_addr);
	







    int sd = UDP_Open(10000);
    int rc = UDP_FillSockAddr(&addrSnd, "localhost", 20000);
    
    while (1) {

        char message[BUFFER_SIZE];
        sprintf(message, "I am the server");

        printf("server:: send message [%s]\n", message);
        rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0) {
        printf("server:: failed to send\n");
        exit(1);
        }

        printf("server:: wait for reply...\n");
        rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
        printf("server:: got reply [size:%d contents:(%s)\n", rc, message);
    }
    return 0;


	
	close(fd);


}