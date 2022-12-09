#include <stdio.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (1000)

void createU(char *message);
void writeU(char *message);
void readU(char *message);

struct sockaddr_in addrRcv, addrSnd;
int rc, sd;
// client code
int main(int argc, char *argv[]) {

    sd = UDP_Open(20000);
    rc = UDP_FillSockAddr(&addrSnd, "localhost", 10000);

   // char message[BUFFER_SIZE];


    char *message = calloc(BUFFER_SIZE, sizeof(char));
    //   MKFS_Creat`0`1`file MKFS_Stat`2 
    //sprintf(message, "MKFS_Write`1`hello_world`0`12");



    //  sprintf(message, "MKFS_Creat`1`1`firstFile");

    // printf("client:: send message [%s]\n", message);
    // rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    // if (rc < 0) {
    //     printf("client:: failed to send\n");
    // //     exit(1);
  // }

    // createU(message);
    // free(message);

    // message = calloc(BUFFER_SIZE, sizeof(char));

    // writeU(message);
    // free(message);

    // message = calloc(BUFFER_SIZE, sizeof(char));

    readU(message);
    free(message);
   

    // sprintf(message, "MKFS_Write`2`hello_world`0`12");

    // printf("client:: send message [%s]\n", message);
    // rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    // if (rc < 0) {
    //     printf("client:: failed to send\n");
    //     exit(1);
    // }





    // sprintf(message,"MKFS_Read`1`0`12");

    // printf("client:: send message [%s]\n", message);
    // rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    // if (rc < 0) {
    //     printf("client:: failed to send\n");
    //     exit(1);
    // }

    

    
    

    

    

//     rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
//     //MFS_Stat_t *stat = (MFS_Stat_t *)(message + 4);
//    int *temp = (int *)message;
//     printf("message returned from server = %d\n", *temp);//message + 4);

    return 0;
}


void createU(char *message) {
    sprintf(message, "MKFS_Creat`1`1`firstFile");
    printf("client:: send message [%s]\n", message);
    rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to send\n");
         exit(1);
    }
    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    int *temp = (int *)message;
    printf("message returned from server = %d\n", *temp);//message + 4);


}

void writeU(char *message) {
    sprintf(message, "MKFS_Write`2`hello_world`0`12");
    printf("client:: send message [%s]\n", message);
    rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }
    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    //MFS_Stat_t *stat = (MFS_Stat_t *)(message + 4);
   int *temp = (int *)message;
    printf("message returned from server = %d\n", *temp);//message + 4);


}

void readU(char *message) {

    sprintf(message,"MKFS_Read`2`0`12");

    printf("client:: send message [%s]\n", message);
    rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }
    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    //MFS_Stat_t *stat = (MFS_Stat_t *)(message + 4);
  // int *temp = (int *)message;
    printf("message returned from server = %s\n", message + 4);


}