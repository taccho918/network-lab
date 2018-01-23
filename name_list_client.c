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

#define PORT_NO 12220
#define BUFSIZE 10000
#define COMMSIZE 100

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
    exit(1);
  }

  bzero((char*) & addr.sin_addr, sizeof(addr.sin_addr));
  memcpy((char*) & addr.sin_addr, (char*)hp->h_addr, hp->h_length);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT_NO);

  // make a connection
  if ((connect(cl_sock, (struct sockaddr *)&addr, sizeof(addr))) == -1) {
    printf("error: connect\n");
    exit(1);
  }

  printf("============ Manual ===========\n");
  printf("%%Q   : Quit\n");
  printf("%%C   : Check how many profiles you have\n");
  printf("%%P n : Print n profile(s)\n");
  printf("%%R file: Read file\n");
  printf("%%W file: Write data to file\n");
  printf("%%F word: Find word\n");
  printf("%%S n: Sort nth column\n");
  printf("===============================\n");

  while(1) {
    memset(command, '\0', COMMSIZE);
    printf("\nInput command: \n");

    // read command input
    if ((read_bufsize = read(0, command, COMMSIZE)) == -1) {
      printf("error: read\n");
      exit(1);
    }

    // send command line to the server from stdin
    if ((send_bufsize = send(cl_sock, command, strlen(command), 0)) == -1) {
      printf("error: send\n");
      exit(1);
    }

    memset(buf, '\0', BUFSIZE);

    // receive results from server
    while(1){
      if ((recv_bufsize = recv(cl_sock, buf, BUFSIZE, 0)) == -1) {
        printf("error: recv\n");
        exit(1);
      } else if (recv_bufsize == 0) {
        break;
      } else if (strcmp(buf, "exit") == 0) {
        printf("EXIT\n");
        exit(0);
      }

      int buf_size = strlen(buf);
      if (buf[buf_size-1] == '\n') break;

      // print content to stdin
      if ((write_bufsize = write(1, buf, BUFSIZE)) == -1) {
        printf("error: write\n");
        exit(1);
      } else if (strcmp(buf, "end") == 0) {
        break;
      }
   }
   if ((write_bufsize = write(1, buf, BUFSIZE)) == -1) {
     printf("error: write\n");
     exit(1);
   }
  }
  close(cl_sock);
}
