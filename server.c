#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "udp.h"
#include "mfs.h"
#include "ufs.h"

#define BUFFER_SIZE (2 * UFS_BLOCK_SIZE)
#define us	(unsigned int)

// FUNCTION PROTOTYPES
int Lookup(int pinum, char *name, unsigned int i_bitMap[]);
int get_allocated(unsigned int pinum, unsigned int i_bitMap[]);
int Shutdown_FS();
int Stat(int inum, MFS_Stat_t *m, unsigned int i_bitMap[]);
int Write(int inum, char *buffer, int offset, int nbytes, unsigned int i_bitMap[], unsigned int d_bitMap[]);
int Creat(int pinum, int type, char *name, unsigned int i_bitMap[], unsigned int d_bitMap[]);
int Read(int inum, char *buffer, int offset, int nbytes, unsigned int i_bitMap[]);
int Unlink(int pinum, char *name, unsigned int i_bitMap[], unsigned int d_bitMap[]);


// HELPER FUNCTIONS
inode_t load_Inode(int inum);
int writeInt(int ret_val, char *ogPointer, struct sockaddr_in *addrRcv, int sd, int *rc);
int allocate_Inode_Bit(unsigned int *i_bitmap);
int allocate_D_Bit(unsigned int d_bitMap[]);
void clear_bit(unsigned int *bitmap, int position);
void set_bit(unsigned int *bitmap, int position);
unsigned int get_bit(unsigned int *bitmap, int position);


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
		//perror("image does not exist\n");
		assert(fd > -1);
		exit(1);
	}
	


	// read in super block
	//read(fd, &s, sizeof(super_t));
	pread(fd, &s, sizeof(super_t), 0);

	// read in inode bitmap
	unsigned int i_bitMap[(s.inode_bitmap_len * UFS_BLOCK_SIZE) / sizeof(unsigned int)];
	pread(fd, i_bitMap, s.inode_bitmap_len * UFS_BLOCK_SIZE, UFS_BLOCK_SIZE * s.inode_bitmap_addr);

	// read in data bitmap
	unsigned int d_bitMap[(s.data_bitmap_len * UFS_BLOCK_SIZE) / sizeof(unsigned int)];
	pread(fd, d_bitMap, s.data_bitmap_len * UFS_BLOCK_SIZE, UFS_BLOCK_SIZE * s.data_bitmap_addr);

	//numInodes = (s.inode_region_len * UFS_BLOCK_SIZE) / sizeof(inode_t);
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
		if (!strcmp("MFS_Lookup", arguments[0])) {
			ret_val = Lookup(atoi(arguments[1]), arguments[2], i_bitMap);	
			printf("ret_val = %d\n", ret_val);
			writeInt(ret_val, ogPointer, &addrRcv, sd, &rc);
		} else if (!strcmp("MFS_Shutdown", arguments[0])) {
			ret_val = Shutdown_FS();
		} else if (!strcmp("MFS_Stat", arguments[0])) {
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
		} else if (!strcmp("MFS_Write", arguments[0])) {
			ret_val = Write(atoi(arguments[1]), arguments[2], atoi(arguments[3]), atoi(arguments[4]), i_bitMap, d_bitMap);
			writeInt(ret_val, ogPointer, &addrRcv, sd, &rc);
		} else if (!strcmp("MFS_Read", arguments[0])) {
			ret_val = Read(atoi(arguments[1]), ogPointer + 4, atoi(arguments[2]), atoi(arguments[3]), i_bitMap);
			if (ret_val == -1) {
				writeInt(ret_val, ogPointer, &addrRcv, sd, &rc);
			} else {
				int *temp = (int *)ogPointer;
				*temp = ret_val;
				rc = UDP_Write(sd, &addrRcv, ogPointer, BUFFER_SIZE);
			}
		} else if (!strcmp("MFS_Creat", arguments[0])) {
			ret_val = Creat(atoi(arguments[1]), atoi(arguments[2]), arguments[3], i_bitMap, d_bitMap);
			writeInt(ret_val, ogPointer, &addrRcv, sd, &rc);
		} else if (!strcmp("MFS_Unlink", arguments[0])) {
			
			ret_val = Unlink(atoi(arguments[1]), arguments[2], i_bitMap, d_bitMap);
			writeInt(ret_val, ogPointer, &addrRcv, sd, &rc);
		} else {
			writeInt(ret_val, ogPointer, &addrRcv, sd, &rc);
		}
	}

	free(ogPointer);
	close(fd);

    return 0;

}


int Unlink(int pinum, char *name, unsigned int i_bitMap[], unsigned int d_bitMap[]) {
	if (pinum >= s.num_inodes) return -1;
	if (!get_bit(i_bitMap, pinum)) return -1;

	// get the parent inode
	inode_t pInode = load_Inode(pinum);

	// IF NOT A DIRECTORY THEN FAIL
	if (pInode.type != MFS_DIRECTORY) return -1;

	int deleted = 0;

	// TRY TO FIND THE DIRECTORY ENTRY CORRESPONDING TO name
	for (int i = 0; i < DIRECT_PTRS && !deleted; ++i) {
		if (pInode.direct[i] == -1) continue;
		dir_ent_t directory_table[UFS_BLOCK_SIZE / sizeof(dir_ent_t)];
		pread(fd, directory_table, UFS_BLOCK_SIZE, pInode.direct[i] * UFS_BLOCK_SIZE);
		for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); ++j) {
			if (directory_table[j].inum == -1) continue;
			// if the name matches then do the unlinking stuff
			if (!strcmp(directory_table[j].name, name)) {
				inode_t inode = load_Inode(directory_table[j].inum);
				// FAIL IF TRYING TO UNLINK DIRECTORY AND IT'S NOT EMPTY
				if (inode.type == MFS_DIRECTORY && (!strcmp(name, ".") || !strcmp(name, ".."))) return -1; 
				if (inode.type == MFS_DIRECTORY && inode.size != 64) return -1;

				// update data bitmap
				for (int k = 0; k < DIRECT_PTRS; k++) {
					// deallocate data blocks
					if (inode.direct[k] != -1) {
						clear_bit(d_bitMap, inode.direct[k] - s.data_region_addr);
					}
				}

				// update inode bitmap
				clear_bit(i_bitMap, directory_table[j].inum);

				// update parent inode and parent directory table
				directory_table[j].inum = -1;
				int empty = 1;
				// look through parent directory table, if empty, delloacte it
				for (int k = 0; k < UFS_BLOCK_SIZE / sizeof(dir_ent_t); ++k) {
					if (directory_table[k].inum != -1) {
						empty = 0;
						break;
					}
				}
				if (empty) {
					clear_bit(d_bitMap, pInode.direct[i]);
					pInode.direct[i] = -1;
				}

				pInode.size -= sizeof(dir_ent_t);

				pwrite(fd, &pInode, sizeof(inode_t), (s.inode_region_addr * UFS_BLOCK_SIZE) + (sizeof(inode_t) * pinum));
				pwrite(fd, directory_table, UFS_BLOCK_SIZE, pInode.direct[i] * UFS_BLOCK_SIZE);
				deleted = 1;
				break;
			}		
		}
	}
	pwrite(fd, d_bitMap, UFS_BLOCK_SIZE * s.data_bitmap_len, s.data_bitmap_addr * UFS_BLOCK_SIZE);
	pwrite(fd, i_bitMap, UFS_BLOCK_SIZE * s.inode_bitmap_len , s.inode_bitmap_addr * UFS_BLOCK_SIZE);

	int return_code = fsync(fd);
	assert(return_code > -1);

	return 0;
}


// returns 0 on successful creation, returns -1 on failure to create
int Creat(int pinum, int type, char *name, unsigned int i_bitMap[], unsigned int d_bitMap[]) {

	if (pinum >= s.num_inodes) return -1;
	if (!get_bit(i_bitMap, pinum)) return -1;
	inode_t inode = load_Inode(pinum);
	

	// check if the filename already exists first 
	for (int i = 0; i < DIRECT_PTRS; ++i) {
		if (inode.direct[i] == -1) continue;
		dir_ent_t directory_table[UFS_BLOCK_SIZE / sizeof(dir_ent_t)];
		pread(fd, directory_table, UFS_BLOCK_SIZE, inode.direct[i] * UFS_BLOCK_SIZE);
		for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); ++j) {
			if (directory_table[j].inum != -1) {
				if (!strcmp(name, directory_table[j].name)) {
					return 0;
				}
			}
		}
	}

	// if no more space for new directory entry, return -1
	if (inode.size == DIRECT_PTRS * UFS_BLOCK_SIZE) return -1;

	int foundEntry = 0;
	dir_ent_t directory_table[UFS_BLOCK_SIZE / sizeof(dir_ent_t)];
	int directory_index = -1;

	// first find free entry to allocate
	for (int i = 0; i < DIRECT_PTRS && !foundEntry; ++i) {
		if (inode.direct[i] == -1) continue;
		pread(fd, directory_table, UFS_BLOCK_SIZE, inode.direct[i] * UFS_BLOCK_SIZE);
		for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); ++j) {
			if (directory_table[j].inum == -1) {
				foundEntry = 1;
				directory_index = i;
				break;
			}
		}
	}
	if (!foundEntry) {
		for (int i = 0; i < DIRECT_PTRS && !foundEntry; ++i) {
			if (inode.direct[i] == -1) {
				unsigned int new_block = allocate_D_Bit(d_bitMap);
				inode.direct[i] = new_block;
				foundEntry = 1;
				directory_index = i;
			}
			//pread(fd, directory_table, UFS_BLOCK_SIZE, new_block * UFS_BLOCK_SIZE);
			for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); ++j) {
				directory_table[j].inum = -1;
			}
		}
	}

	// go through directory table and initialize entry, new inode etc.
	for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); ++j) {
		dir_ent_t current_entry = directory_table[j];
		// found a free entry
		if (current_entry.inum == -1) {
			// find inode spot for new directory/file
			unsigned int new_inum = allocate_Inode_Bit(i_bitMap);
			if (new_inum == -1) return -1;
			// initialize the new inode
			inode_t new_inode;
			new_inode.type = type == 0 ? MFS_DIRECTORY : MFS_REGULAR_FILE;
			new_inode.size = type == 0 ? sizeof(dir_ent_t) * 2 : 0;
			for (int k = 0; k < DIRECT_PTRS; k++) {
				new_inode.direct[k] = -1;
			}
			// if it's a directory, put "." and ".." as directory entries in a data block and write data block in fs image
			if (type == MFS_DIRECTORY) {
				unsigned int new_data_block = allocate_D_Bit(d_bitMap);
				new_inode.direct[0] = new_data_block;
				dir_ent_t new_dir_table[UFS_BLOCK_SIZE / sizeof(dir_ent_t)];
				strcpy(new_dir_table[0].name, ".");
				new_dir_table[0].inum = new_inum;
				strcpy(new_dir_table[1].name, "..");
				new_dir_table[1].inum = pinum;
				for (int index = 2; index < UFS_BLOCK_SIZE / sizeof(dir_ent_t); ++index) {
					new_dir_table[index].inum = -1;
				}
				pwrite(fd, new_dir_table, UFS_BLOCK_SIZE, new_data_block * UFS_BLOCK_SIZE);	
			}
			// update size of parent inode
			inode.size = inode.size + sizeof(dir_ent_t);

			// add new directory entry into parent
			directory_table[j].inum = new_inum;
			strcpy(directory_table[j].name, name);

			// write the new inode into fs image
			pwrite(fd, &new_inode, sizeof(inode_t), (s.inode_region_addr * UFS_BLOCK_SIZE) + (sizeof(inode_t) * new_inum));

			// write the updated data_bitmap and inode_bitmap and the updated parent directory table and update parent inode
			pwrite(fd, &inode, sizeof(inode_t), (s.inode_region_addr * UFS_BLOCK_SIZE) + (sizeof(inode_t) * pinum));
			pwrite(fd, directory_table, UFS_BLOCK_SIZE, inode.direct[directory_index] * UFS_BLOCK_SIZE);
			pwrite(fd, d_bitMap, s.data_bitmap_len * UFS_BLOCK_SIZE, s.data_bitmap_addr * UFS_BLOCK_SIZE);
			pwrite(fd, i_bitMap, s.inode_bitmap_len * UFS_BLOCK_SIZE, s.inode_bitmap_addr * UFS_BLOCK_SIZE);

			int return_code = fsync(fd);
			assert(return_code > -1);
			return 0;
		}

	}
		
	// if file/directory not created, return -1
	return -1;
}

int Read(int inum, char *buffer, int offset, int nbytes, unsigned int i_bitMap[]) {
	
	if (inum >= s.num_inodes) return -1;
	if (!get_bit(i_bitMap, inum)) return -1;

	inode_t inode = load_Inode(inum);

	// if the number of bytes to read is past the end of the file, return -1
	if (offset + nbytes > inode.size) return -1;

	// READ A REGULAR FILE
	if (inode.type == MFS_REGULAR_FILE || inode.type == MFS_DIRECTORY) {
		int directIdx = offset / UFS_BLOCK_SIZE;
		int offset_block = offset % UFS_BLOCK_SIZE;
		int sizeToRead = (UFS_BLOCK_SIZE - offset_block) - nbytes >= 0 ? nbytes : nbytes - (UFS_BLOCK_SIZE - offset_block);
		int leftOver = nbytes - sizeToRead;

		pread(fd, buffer, sizeToRead, (inode.direct[directIdx] * UFS_BLOCK_SIZE) + offset_block);
		if (leftOver > 0) {
			pread(fd, buffer + sizeToRead, leftOver, (inode.direct[directIdx + 1] * UFS_BLOCK_SIZE));
		}
	} else {
		return -1;
	}

	if (inode.type == MFS_DIRECTORY) {
		return 0;
	} else {
		return 1;
	}

}

int Write(int inum, char *buffer, int offset, int nbytes, unsigned int i_bitMap[], unsigned int d_bitMap[]) {
	if (inum >= s.num_inodes) return -1;
	if (!get_bit(i_bitMap, inum)) return -1;

	inode_t inode = load_Inode(inum);
	// if not a regular file or if offset is past the end of file or if writing past the max possible size of file return -1
	if (inode.type == MFS_DIRECTORY || offset > inode.size || offset + nbytes > DIRECT_PTRS * UFS_BLOCK_SIZE) return -1;

	// make enough space available
	int current_space = 0;
	int i = 0;
	while(current_space < offset + nbytes) {
		if (inode.direct[i++] == -1) inode.direct[i] = allocate_D_Bit(d_bitMap);
		current_space += UFS_BLOCK_SIZE;
		i++;
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
	inode.size = offset + nbytes > inode.size ? offset + nbytes : inode.size;

	// writing inode and data bitmap
	pwrite(fd, &inode, sizeof(inode_t), (s.inode_region_addr * UFS_BLOCK_SIZE) + (sizeof(inode_t) * inum));
	pwrite(fd, d_bitMap, s.data_bitmap_len * UFS_BLOCK_SIZE, s.data_bitmap_addr * UFS_BLOCK_SIZE);

	int rc = fsync(fd);
	assert(rc > -1);

	return 0;
}

int Stat(int inum, MFS_Stat_t *m, unsigned int i_bitMap[]) {
	if (inum >= s.num_inodes) return -1;

	if (!get_bit(i_bitMap, inum)) return -1;

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

	if (pinum >= s.num_inodes) return -1;

	// CHECK IF INODE IS ALLOCATED IN INODE BITMAP
	if (!get_bit(i_bitMap, pinum)) return -1;
	
	// CHECK IF INODE IS DIRECTORY
	inode_t inode = load_Inode(pinum);
	if (inode.type != MFS_DIRECTORY) return -1;
	
	// LOOK THROUGH DIRECTORY ENTRIES TO FIND NAME
	for (int i = 0; i<DIRECT_PTRS; ++i) {
		if (inode.direct[i] == -1) continue;
		dir_ent_t directory_table[UFS_BLOCK_SIZE / sizeof(dir_ent_t)];
		pread(fd, directory_table, UFS_BLOCK_SIZE, inode.direct[i] * UFS_BLOCK_SIZE);
		for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); ++j) {
			dir_ent_t current_entry = directory_table[j];
			if (current_entry.inum == -1) continue;
			if (!strcmp(name, current_entry.name)) {
				return current_entry.inum;
			}
		}
	}
	return -1;
}


// HELPER FUNCTIONS


int allocate_Inode_Bit(unsigned int *i_bitMap) {
	for (int i = 0; i < s.num_inodes; ++i) {
		if (get_bit(i_bitMap, i) == 0) {
			set_bit(i_bitMap, i);
			return i;
		}
	}
	return -1;
}

int allocate_D_Bit(unsigned int *d_bitMap) {
	for (int i = 0; i < s.num_data; ++i) {
		if (get_bit(d_bitMap, i) == 0) {
			set_bit(d_bitMap, i);
			return s.data_region_addr + i;
		}
	}
	return -1;
}

inode_t load_Inode(int inum) {
	inode_t inode;
	pread(fd, &inode, sizeof(inode_t), (s.inode_region_addr * UFS_BLOCK_SIZE) + (sizeof(inode_t) * inum));
	return inode;
}


int writeInt(int ret_val, char *ogPointer, struct sockaddr_in *addrRcv, int sd, int *rc) {
		int *ret_message = (int *)ogPointer;
		*ret_message = ret_val;
		*rc = UDP_Write(sd, addrRcv, ogPointer, BUFFER_SIZE);
		return 0;
}


unsigned int get_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   return (bitmap[index] >> offset) & 0x1;
}

void set_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   bitmap[index] |= 0x1 << offset;
}


void clear_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   bitmap[index] &= ~(0x1 << offset);
}
