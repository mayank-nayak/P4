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
			writeInt(ret_val, ogPointer, &addrRcv, sd, &rc);
		} else if (!strcmp("MKFS_Read", arguments[0])) {
			rc = Read(atoi(arguments[1]), ogPointer + 4, atoi(arguments[2]), atoi(arguments[3]), i_bitMap);
			if (ret_val == -1) {
				writeInt(ret_val, ogPointer, &addrRcv, sd, &rc);
			} else {
				int *temp = (int *)ogPointer;
				*temp = ret_val;
				rc = UDP_Write(sd, &addrRcv, ogPointer, BUFFER_SIZE);
			}
		} else if (!strcmp("MKFS_Creat", arguments[0])) {
			ret_val = Creat(atoi(arguments[1]), atoi(arguments[2]), arguments[3], i_bitMap, d_bitMap);
			writeInt(ret_val, ogPointer, &addrRcv, sd, &rc);
		}

		break;
		
	}

	free(ogPointer);
	close(fd);

    return 0;

}


int Unlink(int pinum, char *name, unsigned int i_bitMap[], unsigned int d_bitMap[]) {
	if (pinum >= numInodes) return -1;
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
				if (inode.type == MFS_DIRECTORY && inode.size != 0) return -1; 

				for (int k = 0; k < DIRECT_PTRS; k++) {
					// deallocate data blocks
					if (inode.direct[k] != -1) {
						clear_bit(d_bitMap, inode.direct[k]);
					}
				}
				clear_bit(i_bitMap, directory_table[j].inum);

				directory_table[j].inum = -1;
				pwrite(fd, directory_table, UFS_BLOCK_SIZE, pInode.direct[i] * UFS_BLOCK_SIZE);
				deleted = 1;
				break;
			}


			
		}
	}
	pwrite(fd, d_bitMap, UFS_BLOCK_SIZE * s.data_bitmap_len, s.data_bitmap_addr);
	pwrite(fd, i_bitMap, UFS_BLOCK_SIZE * s.inode_bitmap_len , s.inode_bitmap_addr);


	return 0;
}


// returns 0 on successful creation, returns -1 on failure to create
int Creat(int pinum, int type, char *name, unsigned int i_bitMap[], unsigned int d_bitMap[]) {

	if (pinum >= s.num_inodes) return -1;
	if (!get_bit(i_bitMap, pinum)) return -1;

	inode_t inode = load_Inode(pinum);
	int created = 0;


	// check if the filename already exists first 
	for (int i = 0; i < DIRECT_PTRS; ++i) {
		if (inode.direct[i] == -1) continue;
		dir_ent_t directory_table[UFS_BLOCK_SIZE / sizeof(dir_ent_t)];
		pread(fd, directory_table, UFS_BLOCK_SIZE, inode.direct[i] * UFS_BLOCK_SIZE);
		for (int j = 0; j < UFS_BLOCK_SIZE / sizeof(dir_ent_t); ++j) {
			if (directory_table[j].inum != -1) {
				if (!strcmp(name, directory_table[j].name)) return 0;
			}
		}
	}

	// look for free spot in directory table and assign an entry
	for (int i = 0; i<DIRECT_PTRS && !created; ++i) {
		// looked through all previous directory tables, did not find free spot so directory needs to grow now
		if (inode.direct[i] == -1) {
			// allocate a new data block for parent
			unsigned int new_directory_block = allocate_D_Bit(d_bitMap);
			inode.direct[i] = new_directory_block;
			dir_ent_t temp_table[UFS_BLOCK_SIZE / sizeof(dir_ent_t)];
			for (int index = 0; index < UFS_BLOCK_SIZE / sizeof(dir_ent_t); ++index) {
				temp_table[index].inum = -1;
			}
		}
		dir_ent_t directory_table[UFS_BLOCK_SIZE / sizeof(dir_ent_t)];
		pread(fd, directory_table, UFS_BLOCK_SIZE, inode.direct[i] * UFS_BLOCK_SIZE);
		// look for a free directory entry in the current data block
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
					pwrite(fd, new_dir_table, UFS_BLOCK_SIZE, new_data_block * UFS_BLOCK_SIZE);	
				}
				// update size of parent inode
				inode.size = inode.size + sizeof(dir_ent_t);

				// write the new inode into fs image
				pwrite(fd, &new_inode, sizeof(inode_t), s.inode_region_addr + (sizeof(inode_t) * new_inum));

				// write the updated data_bitmap and inode_bitmap and the updated parent directory table
				pwrite(fd, directory_table, UFS_BLOCK_SIZE, inode.direct[i] * UFS_BLOCK_SIZE);
				pwrite(fd, d_bitMap, s.data_bitmap_len * UFS_BLOCK_SIZE, s.data_bitmap_addr * UFS_BLOCK_SIZE);
				pwrite(fd, i_bitMap, s.inode_bitmap_len * UFS_BLOCK_SIZE, s.inode_bitmap_addr * UFS_BLOCK_SIZE);

				created = 1;
				
				break;
			}

		}
		
	}

	
	fsync(fd);

	return 0;
}

int Read(int inum, char *buffer, int offset, int nbytes, unsigned int i_bitMap[]) {
	if (inum >= s.num_inodes) return -1;
	if (!get_bit(i_bitMap, inum)) return -1;

	inode_t inode = load_Inode(inum);

	// if the number of bytes to read is past the end of the file, return -1
	if (offset + nbytes > inode.size) return -1;

	// READ A REGULAR FILE
	if (inode.type == MFS_REGULAR_FILE) {
		int directIdx = offset / UFS_BLOCK_SIZE;
		int offset_block = offset % UFS_BLOCK_SIZE;
		int sizeToRead = (UFS_BLOCK_SIZE - offset_block) - nbytes >= 0 ? nbytes : nbytes - (UFS_BLOCK_SIZE - offset_block);
		int leftOver = nbytes - sizeToRead;

		pread(fd, buffer, sizeToRead, (inode.direct[directIdx] * UFS_BLOCK_SIZE) + offset_block);
		if (leftOver > 0) {
			pread(fd, buffer + sizeToRead, leftOver, (inode.direct[directIdx + 1] * UFS_BLOCK_SIZE));
		}
	} else {
	// READ A DIRECTORY AND ITS ENTRIES
		pread(fd, buffer, inode.size, (inode.direct[0] * UFS_BLOCK_SIZE));
	}

	return 0;

}

int Write(int inum, char *buffer, int offset, int nbytes, unsigned int i_bitMap[], unsigned int d_bitMap[]) {
	if (inum >= s.num_inodes) return -1;
	if (!get_bit(i_bitMap, inum)) return -1;

	inode_t inode = load_Inode(inum);
	// if not a regular file or if offset is past the end of file return -1
	if (inode.type == MFS_DIRECTORY || offset > inode.size) return -1;

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
		unsigned int new_data_add = allocate_D_Bit(d_bitMap);
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
	inode.size = offset + nbytes > inode.size ? offset + nbytes : inode.size;

	// writing inode and data bitmap
	pwrite(fd, &inode, sizeof(inode_t), s.inode_region_addr + (sizeof(inode_t) * inum));
	pwrite(fd, d_bitMap, s.data_bitmap_len * UFS_BLOCK_SIZE, s.data_bitmap_addr);

	fsync(fd);

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
			if (strcmp(name, current_entry.name) == 0) {
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
	int entry = inum / UFS_BLOCK_SIZE;
	inode_t i_table[UFS_BLOCK_SIZE / sizeof(inode_t)];
	pread(fd, i_table, UFS_BLOCK_SIZE, (s.inode_region_addr + entry) * UFS_BLOCK_SIZE);
	int index = inum % UFS_BLOCK_SIZE;
	return i_table[index];
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
   bitmap[index] &= (~0x1) << offset;
}

// int get_allocated(unsigned int pinum, unsigned int i_bitMap[]) {
// 	int index = pinum / sizeof(unsigned(int));
// 	unsigned int n = i_bitMap[index];
// 	int k = 31 - (pinum % 32);
// 	// unsigned int mask = (unsigned int)1 << k;
// 	// unsigned int masked = n & mask;
// 	// unsigned int inode_bit = masked >> k;
// 	//return inode_bit;

// 	return (n >> k) && 1;
// }


// // returns address of new data block assigned and allocates a bit for the new data block
// int allocate_D_Bit(unsigned int d_bitMap[]) {
// 	unsigned int new_data_add = -1;
// 	for (int i = 0; i < (s.data_bitmap_len * UFS_BLOCK_SIZE) / sizeof(unsigned int) && new_data_add == -1; ++i) {
// 		unsigned int n = d_bitMap[i];
// 		for (int k = 31; k > -1; ++k) {
// 			unsigned int mask = (unsigned int)1 << k;
// 			unsigned int masked = n & mask;
// 			unsigned int inode_bit = masked >> k;
// 			if (inode_bit == 0) {
// 				masked = n | mask;
// 				d_bitMap[i] = masked;
// 				new_data_add = (32 * i) + (31 - k) + s.data_region_addr;
// 				break;
// 			}
// 		}
// 	}
// 	return new_data_add;
// }




// returns address of new inode number and allocated a bit for the new inode
// int allocate_Inode_Bit(unsigned int i_bitMap[]) {
// 	unsigned int new_iNumber = -1;
// 	for (int i = 0; i < (s.inode_bitmap_len * UFS_BLOCK_SIZE) / sizeof(unsigned int) && new_iNumber == -1; ++i) {
// 		unsigned int n = i_bitMap[i];
// 		for (int k = 31; k > -1; ++k) {
// 			unsigned int mask = (unsigned int)1 << k;
// 			unsigned int masked = n & mask;
// 			unsigned int inode_bit = masked >> k;
// 			if (inode_bit == 0) {
// 				masked = n | mask;
// 				i_bitMap[i] = masked;
// 				new_iNumber = (32 * i) + (31 - k);
// 				break;
// 			}
// 		}
// 	}
// 	return new_iNumber;
// }
