#include "mfs.h"
#include "ufs.h"
#include "udp.h"
#include "sys/select.h"
#include <time.h>
#include <stdlib.h>

#define BUFFER_SIZE (2 * UFS_BLOCK_SIZE)

// FUNCTION PROTOTYPES

int MFS_Init(char *hostname, int port);
int MFS_Lookup(int pinum, char *name);
int MFS_Stat(int inum, MFS_Stat_t *m);
int MFS_Write(int inum, char *buffer, int offset, int nbytes);
int MFS_Read(int inum, char *buffer, int offset, int nbytes);
int MFS_Creat(int pinum, int type, char *name);
int MFS_Unlink(int pinum, char *name);
int MFS_Shutdown();

struct sockaddr_in addrSnd, addrRcv;
int sd, rc;
char sep = '`';
//int fd;
fd_set set;
struct timeval timeout;

int MFS_Init(char *hostname, int port) {
    int MIN_PORT = 20000;
    int MAX_PORT = 40000;

    srand(time(0));
    int port_num = (rand() % (MAX_PORT - MIN_PORT) + MIN_PORT);

    // Bind random client port number

    sd = UDP_Open(port_num);

    rc = UDP_FillSockAddr(&addrSnd, hostname, port);
    if (rc < 0) return -1;
    // fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // if (fd < 0) return -1;
    // rc = bind(fd, (const struct sockaddr*)&addrRcv, sizeof(struct sockaddr_in));
    // if (rc < 0) return -1;
    // rc = connect(fd, (const struct sockaddr*)&addrRcv, sizeof(struct sockaddr_in));
    // if (rc < 0) return -1;
    // timeout.tv_sec = 5;
    // timeout.tv_usec = 0;
    // FD_ZERO(&set);
    // FD_SET(fd, &set);
    return 0;
}

int MFS_Lookup(int pinum, char *name) {

    if (strlen(name) > 27) return -1;

    char message[BUFFER_SIZE];
    char* func = "MFS_Lookup";
    snprintf(message, sizeof(message), "%s%c%i%c%s", func, sep, pinum, sep, name);
    int rc;
  //  while (1) {
        rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0) {
            printf("client:: failed to send lookup\n");
            exit(1);
        }
    //     rc = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
    //     if (rc < 0) exit(1);
    //     if (rc == 0) {
    //         continue;
    //     } else break;
    // }
  //  while ((select(FD_SETSIZE, &set, NULL, NULL, &timeout)) < 1) ;
       

    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to recieve\n");
        return -1;
    }

    return *(int*)message;
}

int MFS_Stat(int inum, MFS_Stat_t *m) {
    char message[BUFFER_SIZE];
    char* func = "MFS_Stat";
    snprintf(message, sizeof(message), "%s%c%i", func, sep, inum);
    int rc;
    printf("message to send = %s\n", message);
   // while (1) {
        rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0) {
            printf("client:: failed to send\n");
            exit(1);
        }
    //     rc = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
    //     if (rc < 0) exit(1);
    //     if (rc == 0) {
    //         continue;
    //     } else break;
    // }

    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    if (rc < 0) {
        return -1;
    }

    m->type = *(int*)(message+4);
    m->size = *(int*)(message+8);
    return 0;    
}

int MFS_Write(int inum, char *buffer, int offset, int nbytes) {
    if (nbytes > 4096) return -1;
    char message[BUFFER_SIZE];
    char* func = "MFS_Write";
    snprintf(message, sizeof(message), "%s%c%i%c%i%c%i%c", func, sep, inum, sep, nbytes, sep, offset, sep);
    memcpy(message + strlen(message) , buffer, nbytes);

    int rc;
  //  while (1) {
        rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0) {
            printf("client:: failed to send\n");
            exit(1);
        }
    //     rc = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
    //     if (rc < 0) exit(1);
    //     if (rc == 0) {
    //         continue;
    //     } else break;
    // }

    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    if (rc < 0) {
        return -1;
    }
    printf("MFS Write complete\n");

    return *(int*)message;
}

int MFS_Read(int inum, char *buffer, int offset, int nbytes) {
    if (nbytes > 4096) return -1;
    char message[BUFFER_SIZE];
    char* func = "MFS_Read";
    
    snprintf(message, sizeof(message), "%s%c%i%c%i%c%i", func, sep, inum, sep, offset, sep, nbytes);

    int rc;
   // while (1) {
        rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0) {
            printf("client:: failed to send\n");
            exit(1);
        }
    //     rc = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
    //     if (rc < 0) exit(1);
    //     if (rc == 0) {
    //         continue;
    //     } else break;
    // }

    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    if (rc < 0) {
        return -1;
    }

    if (*(int*)message < 0) return -1;

    // 1 is file 0 is directory
    // array of directory structures
    if (*(int*)message == 0) {
        memcpy(buffer, message + 4, nbytes);
    }
    if (*(int*)message == 1) {
        memcpy(buffer, message + 4, nbytes);
    }

    return 0;
}

int MFS_Creat(int pinum, int type, char *name) {

    if (strlen(name) > 27) return -1;

    char message[BUFFER_SIZE];
    char* func = "MFS_Creat";
    snprintf(message, sizeof(message), "%s%c%i%c%i%c%s", func, sep, pinum, sep, type, sep, name);

    int rc;
   // while (1) {
        rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0) {
            printf("client:: failed to send\n");
            exit(1);
        }
    //     rc = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
    //     if (rc < 0) exit(1);
    //     if (rc == 0) {
    //         continue;
    //     } else break;
    // }

    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to recieve\n");
        return -1;
    }
    printf("the message returned is %d\n", *(int*)message);
    return *(int*)message;
}

int MFS_Unlink(int pinum, char *name) {

    if (strlen(name) > 27) return -1;

    char message[BUFFER_SIZE];
    char* func = "MFS_Unlink";
    snprintf(message, sizeof(message), "%s%c%i%c%s", func, sep, pinum, sep, name);

    int rc;
    //while (1) {
        rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
        if (rc < 0) {
            printf("client:: failed to send\n");
            exit(1);
        }
        // rc = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
        // if (rc < 0) exit(1);
        // if (rc == 0) {
        //     continue;
        // } else break;
   // }

    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    if (rc < 0) {
        return -1;
    }
    
    return *(int*)message;
}

int MFS_Shutdown() {
    char message[BUFFER_SIZE];
    char* func = "MFS_Shutdown";
    snprintf(message, sizeof(message), "%s", func);

    int rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    printf("rc = %d\n", rc);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    } 
    printf("called shutdown\n");
    
    UDP_Close(sd);
    return 0;  
}
	
	

	
