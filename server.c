#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "udp.h"
#include "mfs.h"
#include "ufs.h"

#define BUFFER_SIZE (1000)

int main(int argc, char *argv[]) {
	
	if (argc != 3) {
		printf("usage: server [portnum] [file-system-image]\n");
		exit(1);
	}

	// get port number
	int port = atoi(argv[1]);
	if (port == 0) exit(1);
	

	// get file system image
	int fd = open(argv[2], O_RDWR);
	// if file system image doesn't exist, then exit
	if (fd < 0) {
		perror("image does not exist\n");
		exit(1);
	}
	
	// read in super block
	super_t s;
	read(fd, &s, sizeof(super_t));

	// read in inode bitmap
	unsigned int i_bitMap[(s.inode_bitmap_len * UFS_BLOCK_SIZE) / sizeof(unsigned int)];
	pread(fd, i_bitMap, s.inode_bitmap_len * UFS_BLOCK_SIZE, UFS_BLOCK_SIZE * s.inode_bitmap_addr);

	// read in data bitmap
	unsigned int d_bitMap[(s.data_bitmap_len * UFS_BLOCK_SIZE) / sizeof(unsigned int)];
	pread(fd, d_bitMap, s.data_bitmap_len * UFS_BLOCK_SIZE, UFS_BLOCK_SIZE * s.data_bitmap_addr);

	// read in inode region
	inode_t inode_table[(s.data_bitmap_len * UFS_BLOCK_SIZE) / sizeof(inode_t)];
	pread(fd, inode_table, s.inode_region_len * UFS_BLOCK_SIZE, UFS_BLOCK_SIZE * s.inode_region_addr);



	struct sockaddr_in addrSnd, addrRcv;

    int sd = UDP_Open(port);
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

int lookup() {

	
	return 0;
}