#include <stdio.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (1000)

void createU(char *message, char *command);
void writeU(char *message, char *command);
void readU(char *message, char *command);
void unlinkU(char *message, char *command);
void statU(char *message, char *command);
void shutdownU(char *message, char *command);
void lookupU(char *message, char *command);

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

    createU(message, "MKFS_Creat`0`0`firstDir");
    createU(message, "MKFS_Creat`0`0`firstDir");
    createU(message, "MKFS_Creat`1`1`firstFile");
    writeU(message, "MKFS_Write`2`hello_world`0`12");
    readU(message, "MKFS_Read`2`0`12");
    lookupU(message, "MKFS_Lookup`1`firstFile");
    shutdownU(message, "MKFS_Shutdown");


    //  sprintf(message, "MKFS_Creat`1`1`firstFile");

    // printf("client:: send message [%s]\n", message);
    // rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    // if (rc < 0) {
    //     printf("client:: failed to send\n");
    // //     exit(1);
  // }

    

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


void createU(char *message, char *command) {
sprintf(message,"%s", command);
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

void writeU(char *message, char *command) {
sprintf(message,"%s", command);
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

void readU(char *message, char *command) {

sprintf(message,"%s", command);

    printf("client:: send message [%s]\n", message);
    rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }
    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    //MFS_Stat_t *stat = (MFS_Stat_t *)(message + 4);
  printf("message returned from server = %s\n", message + 4);
    int *temp = (int *)message;
    printf("message returned from server = %d\n", *temp);//message + 4);



}

void unlinkU(char *message, char *command) {
sprintf(message,"%s", command);

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

void statU(char *message, char *command) {
sprintf(message,"%s", command);

    printf("client:: send message [%s]\n", message);
    rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }
   rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    MFS_Stat_t *stat = (MFS_Stat_t *)(message + 4);
  
    printf("message returned from server = %d\n", stat->type);
}

void shutdownU(char *message, char *command) {
sprintf(message,"%s", command);

    printf("client:: send message [%s]\n", message);
    rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
        printf("client:: failed to send\n");
        exit(1);
    }
}

void lookupU(char *message, char *command) {
    sprintf(message,"%s", command);

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