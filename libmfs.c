#include "mfs.h"
#include "ufs.h"
#include "udp.h"

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
int sd;
char sep = '`';

int MFS_Init(char *hostname, int port) {
    sd = UDP_Open(20000);
    int rc = UDP_FillSockAddr(&addrSnd, hostname, port);
    if (rc < 0) return -1;
    return 0;
}

int MFS_Lookup(int pinum, char *name) {
    char message[BUFFER_SIZE];
    char* func = "MFS_Lookup";
    snprintf(message, sizeof(message), "%s%c%i%c%s", func, sep, pinum, sep, name);

    int rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }

    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    if (rc < 0) {
        return -1;
    }

    return *(int*)message;
}

int MFS_Stat(int inum, MFS_Stat_t *m) {
    char message[BUFFER_SIZE];
    char* func = "MFS_Stat";
    snprintf(message, sizeof(message), "%s%c%i%c%i%i", func, sep, inum, sep, m->type, m->size);

    int rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }

    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    if (rc < 0) {
        return -1;
    }

    m = (MFS_Stat_t*)((int*)message + 1); 
    return 0;    
}

int MFS_Write(int inum, char *buffer, int offset, int nbytes) {
    if (nbytes > 4096) return -1;
    char message[BUFFER_SIZE];
    char* func = "MFS_Write";
    snprintf(message, sizeof(message), "%s%c%i%c%s%c%i%c%i", func, sep, inum, sep, buffer, sep, offset, sep, nbytes);

    int rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }

    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    if (rc < 0) {
        return -1;
    }

    return *(int*)message;
}

int MFS_Read(int inum, char *buffer, int offset, int nbytes) {
    if (nbytes > 4096) return -1;
    char message[BUFFER_SIZE];
    char* func = "MFS_Read";
    snprintf(message, sizeof(message), "%s%c%i%c%c%c%i", func, sep, inum, sep, offset, sep, nbytes);

    int rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }

    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    if (rc < 0) {
        return -1;
    }

    if (*(int*)message < 0) return -1;

    // 1 is file 0 is directory
    // array of directory structures
    if (*(int*)message == 0) {
        memcpy(buffer, (int*)message + 1, nbytes);
    }
    if (*(int*)message == 1) {
        memcpy(buffer, (int*)message + 1, nbytes);
    }

    return 0;
}

int MFS_Creat(int pinum, int type, char *name) {
    char message[BUFFER_SIZE];
    char* func = "MFS_Creat";
    snprintf(message, sizeof(message), "%s%c%i%c%i%c%s", func, sep, pinum, sep, type, sep, name);

    int rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }

    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    if (rc < 0) {
        return -1;
    }
    
    return *(int*)message;
}

int MFS_Unlink(int pinum, char *name) {
    char message[BUFFER_SIZE];
    char* func = "MFS_Unlink";
    snprintf(message, sizeof(message), "%s%c%i%c%s", func, sep, pinum, sep, name);

        int rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }

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
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    } 

    return 0;  
}
