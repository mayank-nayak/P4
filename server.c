#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "udp.h"
#include "mfs.h"
#include "ufs.h"

#define BUFFER_SIZE (1000)

// FUNCTION PROTOTYPES
int lookup(int pinum, char *name, unsigned int i_bitMap[]);
int get_allocated(unsigned int n);

///////////////////////////////////////////// METADATA //////////////////////////////////
// super block
super_t s;
// number of inodes
int numInodes;

////////////////////////////////////////////////////////////////////////////////////////



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
	read(fd, &s, sizeof(super_t));

	// read in inode bitmap
	unsigned int i_bitMap[(s.inode_bitmap_len * UFS_BLOCK_SIZE) / sizeof(unsigned int)];
	pread(fd, i_bitMap, s.inode_bitmap_len * UFS_BLOCK_SIZE, UFS_BLOCK_SIZE * s.inode_bitmap_addr);

	// read in data bitmap
	unsigned int d_bitMap[(s.data_bitmap_len * UFS_BLOCK_SIZE) / sizeof(unsigned int)];
	pread(fd, d_bitMap, s.data_bitmap_len * UFS_BLOCK_SIZE, UFS_BLOCK_SIZE * s.data_bitmap_addr);

	numInodes = (s.inode_region_len * UFS_BLOCK_SIZE) / sizeof(inode_t);

	// read in inode region
	inode_t inode_table[numInodes];
	pread(fd, (void *)inode_table, s.inode_region_len * UFS_BLOCK_SIZE, UFS_BLOCK_SIZE * s.inode_region_addr);



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
		


		free(freeMessage);
		break;

	}

	close(fd);

    return 0;

}

int lookup(int pinum, char *name, unsigned int i_bitMap[], inode_t inode_table[]) {

	if (pinum >= numInodes) return -1;

	// CHECK IF INODE IS ALLOCATED IN INODE BITMAP
	int index = pinum / sizeof(unsigned(int));
	unsigned int entry = i_bitMap[index];

	if (!get_allocated(pinum)) return -1;
	
	inode_t inode = inode_table[pinum];
	if (inode.type != UFS_DIRECTORY) return -1;

	for (int i = 0; i<DIRECT_PTRS; ++i) {
		if (inode.direct[i] == -1) continue;
		
		void *directory_table = inode.direct[i];
		
	}

	return 0;
}











// HELPER FUNCTIONS

int get_allocated(unsigned int n) {
	int k = 31 - (n % 32);
	unsigned int mask = (unsigned int)1 << k;
	unsigned int masked = n & mask;
	unsigned int inode_bit = masked >> k;
	return inode_bit;
}