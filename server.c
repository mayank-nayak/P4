#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "udp.h"
#include "mfs.h"
#include "ufs.h"

#define BUFFER_SIZE (2 * UFS_BLOCK_SIZE)

// FUNCTION PROTOTYPES
int Lookup(int pinum, char *name, unsigned int i_bitMap[]);
int get_allocated(unsigned int pinum, unsigned int i_bitMap[]);
int Shutdown_FS();
int Stat(int inum, MFS_Stat_t *m, unsigned int i_bitMap[]);
int writeInt(int ret_val, char *ogPointer, struct sockaddr_in *addrRcv, int sd, int *rc);
int Write(int inum, char *buffer, int offset, int nbytes, unsigned int i_bitMap[], unsigned int d_bitMap[]);
inode_t load_Inode(int inum);

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
	// inode_t inode_table[numInodes];
	// pread(fd, (void *)inode_table, s.inode_region_len * UFS_BLOCK_SIZE, UFS_BLOCK_SIZE * s.inode_region_addr);



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
		char *arguments[5];
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
			ret_val = Lookup(atoi(arguments[1]), arguments[2], i_bitMap);	
			printf("ret_val = %d\n", ret_val);
			writeInt(ret_val, ogPointer, &addrRcv, sd, &rc);

		} else if (!strcmp("MKFS_Shutdown", arguments[0])) {
			ret_val = Shutdown_FS();
			
		} else if (!strcmp("MKFS_Stat", arguments[0])) {
			MFS_Stat_t m;
			ret_val = Stat(atoi(arguments[1]), &m, i_bitMap);
			if (ret_val == -1) {
				writeInt(ret_val, ogPointer, &addrRcv, sd, &rc);
			} else {
				int *structBuffer = (int *)ogPointer;
				*structBuffer = 0;
				structBuffer++;
				*structBuffer = m.type;
				structBuffer = structBuffer + 1;
				*structBuffer = m.size;
				rc = UDP_Write(sd, &addrRcv, ogPointer, BUFFER_SIZE);
			}
		} else if (!strcmp("MKFS_Write", arguments[0])) {
			ret_val = Write(atoi(arguments[1]), arguments[2], atoi(arguments[3]), atoi(arguments[4]), i_bitMap, d_bitMap);
			return 0;
		}

		break;
		
	}

	free(ogPointer);
	close(fd);

    return 0;

}

int Write(int inum, char *buffer, int offset, int nbytes, unsigned int i_bitMap[], unsigned int d_bitMap[]) {
	if (inum >= numInodes) return -1;
	if (!get_allocated(inum, i_bitMap)) return -1;

	inode_t inode = load_Inode(inum);
	// if not a regular file or if offset is past the end of file return -1
	if (inode.type == UFS_DIRECTORY || offset > inode.size) return -1;

	// check if the file has enough space
	int allocated = 0;
	for (int i = 0; i < DIRECT_PTRS; ++i) {
		if (inode.direct[i] != -1) {
			allocated += UFS_BLOCK_SIZE;
		}
	}
	int available_space = allocated - offset;

	if (available_space < nbytes) {
		// need to allocate more space for file
		unsigned int new_data_add = -1;
		for (int i = 0; i < (s.data_bitmap_len * UFS_BLOCK_SIZE) / sizeof(unsigned int) && new_data_add == -1; ++i) {
			unsigned int n = d_bitMap[i];
			for (int k = 31; k > -1; ++k) {
				unsigned int mask = (unsigned int)1 << k;
				unsigned int masked = n & mask;
				unsigned int inode_bit = masked >> k;
				if (inode_bit == 0) {
					masked = n | mask;
					d_bitMap[i] = masked;
					new_data_add = (32 * i) + (31 - k) + s.data_region_addr;
					break;
				}
			}
		}

		// no more space left
		if (new_data_add == -1) return -1;

		for (int i = 0; i < DIRECT_PTRS; ++i) {
			if (inode.direct[i] == -1) {
				inode.direct[i] = new_data_add;
			}
		}
	}

	// writing the actual data
	int directIdx = offset / UFS_BLOCK_SIZE;
	int offset_block = offset % 4096;

	int sizeToWrite = (UFS_BLOCK_SIZE - offset_block) - nbytes >= 0 ? nbytes : nbytes - (UFS_BLOCK_SIZE - offset_block);
	int leftOver = nbytes - sizeToWrite;

	pwrite(fd, buffer, sizeToWrite, (inode.direct[directIdx] * UFS_BLOCK_SIZE) + offset_block);
	if (leftOver > 0) {
		pwrite(fd, buffer + (sizeToWrite), leftOver, inode.direct[directIdx + 1] * UFS_BLOCK_SIZE);
	}
	
	// update size
	inode.size = offset + nbytes;

	// writing inode and data bitmap
	pwrite(fd, &inode, sizeof(inode_t), s.inode_region_addr + (sizeof(inode_t) * inum));
	pwrite(fd, d_bitMap, s.data_bitmap_len * UFS_BLOCK_SIZE, s.data_bitmap_addr);

	fsync(fd);

	return 0;
}

int Stat(int inum, MFS_Stat_t *m, unsigned int i_bitMap[]) {
	if (inum >= numInodes) return -1;

	if (!get_allocated(inum, i_bitMap)) return -1;

	inode_t inode = load_Inode(inum);
	m->type = inode.type;
	m->size = inode.size;
	return 0;
}

int Shutdown_FS() {
	fsync(fd);
	exit(0);
}

int Lookup(int pinum, char *name, unsigned int i_bitMap[]) {

	if (pinum >= numInodes) return -1;

	// CHECK IF INODE IS ALLOCATED IN INODE BITMAP
	if (!get_allocated(pinum, i_bitMap)) return -1;
	
	// CHECK IF INODE IS DIRECTORY
	inode_t inode = load_Inode(pinum);
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


inode_t load_Inode(int inum) {
	int entry = inum / UFS_BLOCK_SIZE;
	inode_t i_table[UFS_BLOCK_SIZE / sizeof(inode_t)];
	pread(fd, i_table, UFS_BLOCK_SIZE, (s.inode_region_addr + entry) * UFS_BLOCK_SIZE);
	int index = inum % UFS_BLOCK_SIZE;
	return i_table[index];
}

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






