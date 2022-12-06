#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "udp.h"
#include "mfs.h"
#include "ufs.h"

#define BUFFER_SIZE (1000)

// FUNCTION PROTOTYPES
int Lookup(int pinum, char *name, unsigned int i_bitMap[], inode_t inode_table[]);
int get_allocated(unsigned int pinum, unsigned int i_bitMap[]);
int Shutdown_FS();
int Stat(int inum, MFS_Stat_t *m, unsigned int i_bitMap[], inode_t inode_table[]);
int writeInt(int ret_val, char *ogPointer, struct sockaddr_in *addrRcv, int sd, int *rc);

///////////////////////////////////////////// METADATA //////////////////////////////////
// super block
super_t s;
// number of inodes
int numInodes;

////////////////////////////////////////////////////////////////////////////////////////

int fd;


int main(int argc, char *argv[]) {
	
	if (argc != 3) {
		printf("usage: server [portnum] [file-system-image]\n");
		exit(1);
	}

	// get port number
	int port = atoi(argv[1]);
	if (port == 0) exit(1);
	

	// get file system image
	fd = open(argv[2], O_RDWR);
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

    int rc, sd = UDP_Open(port);
    char *message;
	char *ogPointer;

    while (1) {
		// receiving command
		//char message[BUFFER_SIZE];
		message = malloc(sizeof(char) * BUFFER_SIZE);
		ogPointer = message;
		rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
		if (rc < 0) {
			perror("failed to read\n");
			exit(1);
		}
		// parsing the client's commands
		int argNum = 0;
		char *arguments[4];
		char *token = "";

		while(token != NULL) {
			token = strsep(&message, "`");
			if (token == NULL || *token == '\0') continue;
			arguments[argNum] = token;
			argNum++;
    	}

		int ret_val = -1;


		// processing the client's commands
		if (!strcmp("MKFS_Lookup", arguments[0])) {
			ret_val = Lookup(atoi(arguments[1]), arguments[2], i_bitMap, inode_table);	
			writeInt(ret_val, ogPointer, &addrRcv, sd, &rc);

		} else if (!strcmp("MKFS_Shutdown", arguments[0])) {
			ret_val = Shutdown_FS();
			
		} else if (!strcmp("MKFS_Stat", arguments[0])) {
			MFS_Stat_t m;
			ret_val = Stat(atoi(arguments[1]), &m, i_bitMap, inode_table);
			if (ret_val == -10) {
				writeInt(ret_val, ogPointer, &addrRcv, sd, &rc);
			} else {
				int *structBuffer = (int *)ogPointer;
				*structBuffer = m.type;
				structBuffer = structBuffer + 1;
				*structBuffer = m.size;
				rc = UDP_Write(sd, &addrRcv, ogPointer, BUFFER_SIZE);
			}

		} 



		break;
		// sending the return value back to client
		

	}

	free(ogPointer);
	close(fd);

    return 0;

}





int Stat(int inum, MFS_Stat_t *m, unsigned int i_bitMap[], inode_t inode_table[]) {
	if (inum >= numInodes) return -10;

	if (!get_allocated(inum, i_bitMap)) return -10;

	inode_t inode = inode_table[inum];
	m->type = inode.type;
	m->size = inode.size;
	return 10;
}

int Shutdown_FS() {
	fsync(fd);
	exit(0);
}

int Lookup(int pinum, char *name, unsigned int i_bitMap[], inode_t inode_table[]) {

	if (pinum >= numInodes) return -1;

	// CHECK IF INODE IS ALLOCATED IN INODE BITMAP
	if (!get_allocated(pinum, i_bitMap)) return -1;
	
	// CHECK IF INODE IS DIRECTORY
	inode_t inode = inode_table[pinum];
	if (inode.type != UFS_DIRECTORY) return -1;

	// LOOK THROUGH DIRECTORY ENTRIES TO FIND NAME
	for (int i = 0; i<DIRECT_PTRS; ++i) {
		if (inode.direct[i] == -1) continue;
		dir_ent_t directory_table[UFS_BLOCK_SIZE / sizeof(dir_ent_t)];
		pread(fd, directory_table, UFS_BLOCK_SIZE, inode.direct[i] * UFS_BLOCK_SIZE);
		for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); ++j) {
			dir_ent_t current_entry = directory_table[j];
			if (current_entry.inum == -1) continue;
			if (strcmp(name, current_entry.name) == 0) {
				return current_entry.inum;
			}
		}
	}

	return -1;
}











// HELPER FUNCTIONS

int get_allocated(unsigned int pinum, unsigned int i_bitMap[]) {
	int index = pinum / sizeof(unsigned(int));
	unsigned int n = i_bitMap[index];
	int k = 31 - (n % 32);
	unsigned int mask = (unsigned int)1 << k;
	unsigned int masked = n & mask;
	unsigned int inode_bit = masked >> k;
	return inode_bit;
}

int writeInt(int ret_val, char *ogPointer, struct sockaddr_in *addrRcv, int sd, int *rc) {
		int *ret_message = (int *)ogPointer;
		*ret_message = ret_val;
		*rc = UDP_Write(sd, addrRcv, ogPointer, BUFFER_SIZE);
		return 0;
}