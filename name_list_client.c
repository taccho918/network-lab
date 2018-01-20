/********************************************
name_list_client:
 サーバにコマンドを送り，処理結果を受け取り，表示する
********************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT_NO 12225
#define BUFSIZE 10000
#define COMMSIZE 100
#define MAX_STR_LEN 69

struct date {
  int y;
  int m;
  int d;
};

struct profile {
  int number;
  char name[MAX_STR_LEN+1];
  struct date birthday;
  char address[MAX_STR_LEN+1];
  char *comment;
};

void print_profile(struct profile *p)
{
  char date[11];

  printf("number:   %d\n", p->number);
  printf("name:     %s\n", p->name);
  printf("birthday: %d-%d-%d\n", p->birthday.y, p->birthday.m, p->birthday.d);
  printf("address:  %s\n", p->address);
  printf("comment:  %s\n", p->comment);
}

int main(int argc, char* argv[])
{
  struct sockaddr_in addr; //dest address
  int cl_sock;  //socket descriptor
  struct hostent* hp;

  char command[COMMSIZE];
  char buf[BUFSIZE]; // to print the results

  int read_bufsize;
  int send_bufsize;
  int recv_bufsize;
  int write_bufsize;

  // get IP from host name
  if ((hp = gethostbyname(argv[1])) == NULL) {
    printf("error: gethostbyname\n");
    return (1);
  }

  // create socket
  cl_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (cl_sock == -1) {
    printf("error: sock\n");
    return (1);
  }

  bzero((char*) & addr.sin_addr, sizeof(addr.sin_addr));
  memcpy((char*) & addr.sin_addr, (char*)hp->h_addr, hp->h_length);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT_NO); 

  // make a connection
  if ((connect(cl_sock, (struct sockaddr *)&addr, sizeof(addr))) == -1) {
    printf("error: connect\n");
    return (1);
  }

  while(1) {
    memset(command, '\0', COMMSIZE);

    printf("\nInput command: \n");
    // read command input
    read_bufsize = read(0, command, COMMSIZE);
    if (read_bufsize == -1) {
      printf("error: read\n");
      return 1;
    }

    // send command line to the server from stdin
    send_bufsize = send(cl_sock, command, strlen(command), 0);
    if (send_bufsize == -1) {
      printf("error: send\n");
      return 1;
    }

    memset(buf, '\0', BUFSIZE);
    
    // receive results from server
    recv_bufsize = recv(cl_sock, buf, BUFSIZE, 0);
    if (recv_bufsize == -1) {
      printf("error: recv\n");
      return 1;
    } 
   
    if (strcmp(buf, "exit") == 0) {
      printf("EXIT\n");
      exit(0);
    }
   
    // print content to stdin
    write_bufsize = write(1, buf, BUFSIZE);
    if (write_bufsize == -1) {
      printf("error: write\n");
      return 1;
    }
  }
  close(cl_sock);
  printf("close client socket");
}
