#include <stdio.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (1000)

// client code
int main(int argc, char *argv[]) {
    struct sockaddr_in addrRcv, addrSnd;

    int sd = UDP_Open(20000);
    int rc = UDP_FillSockAddr(&addrSnd, "localhost", 10000);

    char message[BUFFER_SIZE];
    //   MKFS_Creat`0`1`file
    sprintf(message, "MKFS_Stat`1");

    printf("client:: send message [%s]\n", message);
    rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }

    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    MFS_Stat_t *stat = (MFS_Stat_t *)(message + 4);
 
    printf("message returned from server = %d\n", stat->size);
    return 0;
}