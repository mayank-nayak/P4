#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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



	struct sockaddr_in addrRcv;

    int sd = UDP_Open(port);
    int rc;

    while (1) {
		// receiving command
		//char message[BUFFER_SIZE];
		char *message = malloc(sizeof(char) * BUFFER_SIZE);
		char *freeMessage = message;
		rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);

		printf("message in server: %s\n", message);

		// processing command
		int argNum = 0;
		char *arguments[4];
		char *token = "";

		while(token != NULL) {
			token = strsep(&message, "`");
			if (token == NULL || *token == '\0') continue;
			arguments[argNum] = token;
			argNum++;
    	}
		
		// if (strcmp(arguments[0], "MFS_Lookup")) {
			
		// }
		

		free(freeMessage);
		return rc;

	}


    return 0;


	
	close(fd);


}

int lookup() {

	
	return 0;
}