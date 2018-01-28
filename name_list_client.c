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

#define PORT_NO 12222
#define BUFSIZE 10000

void process_print(int cl_sock)
{
  int recv_size;
  char buf[BUFSIZE];

  while (1) {
    memset(buf, 0, BUFSIZE);

    recv_size = recv(cl_sock, buf, BUFSIZE, 0);
    if (recv_size == -1) {
      perror("recv");
      exit(1);
    } else if (recv_size == 0) {
      close(cl_sock);
      break;
    } else {
      // do nothing
    }
    if ((recv_size = write(1, buf, sizeof(buf))) == -1) {
      perror("write");
      exit(1);
    }
  }
}

void process_else(int cl_sock)
{
  int recv_size;
  char buf[BUFSIZE];
  int buflen;

  while (1) {
    memset(buf, 0, BUFSIZE);
    recv_size = recv(cl_sock, buf, BUFSIZE, 0);
    buflen = strlen(buf);

    if (recv_size == -1) {
      perror("recv");
      exit(1);
    } else if (recv_size == 0) {
      break;
    } else {
      // do nothing
    }

    if (strcmp(buf, "exit") == 0) {
      printf("EXIT\n");
      exit(0);
    }

    if ((recv_size = write(1, buf, sizeof(buf))) == -1) {
      perror("write");
      exit(1);
    }

    if (buf[buflen-1] == '\n') break;
  }
}


int main(int argc, char* argv[])
{
  struct sockaddr_in addr; //dest address
  int cl_sock;  //socket descriptor
  struct hostent* hp;

  char buf[BUFSIZE]; // for data process

  int read_bufsize;
  int recv_bufsize;
  int write_size;

  // get IP from host name
  if ((hp = gethostbyname(argv[1])) == NULL) {
    perror("gethostbyname");
    return 1;
  }

  // create socket
  cl_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (cl_sock == -1) {
    perror("sock");
    return 1;
  }

  bzero((char*) & addr.sin_addr, sizeof(addr.sin_addr));
  memcpy((char*) & addr.sin_addr, (char*)hp->h_addr, hp->h_length);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT_NO);

  // make a connection
  if ((connect(cl_sock, (struct sockaddr *)&addr, sizeof(addr))) == -1) {
    perror("connect");
    return 1;
  }

  printf("================ Manual ================\n");
  printf("%%Q   : Quit\n");
  printf("%%C   : Check how many profiles you have\n");
  printf("%%P n : Print n profile(s)\n");
  printf("%%R file: Read file\n");
  printf("%%W file: Write data to file\n");
  printf("%%F word: Find word\n");
  printf("%%S n: Sort nth column\n");
  printf("========================================\n");

  while(1) {
    memset(buf, '\0', BUFSIZE);
    printf("\nInput command or CSV data: \n");

    // read command input
    if ((read_bufsize = read(0, buf, BUFSIZE)) == -1) {
      perror("read");
      return 1;
    }

    // send command line to the server from stdin
    if ((read_bufsize = send(cl_sock, buf, strlen(buf), 0)) == -1) {
      perror("send");
      return 1;
    }

    switch (buf[1]) {
      case 'P': process_print(cl_sock);  printf("print\n"); break;
      case 'Q': process_else(cl_sock); break;
      case 'C': process_else(cl_sock); break;
      case 'R': process_else(cl_sock); break;
      case 'W': process_else(cl_sock); break;
      case 'F': process_else(cl_sock); break;
      case 'S': process_else(cl_sock); break;
      default:
        printf("command is not listed here\n");
        break;
    }
  }
  close(cl_sock);
}
