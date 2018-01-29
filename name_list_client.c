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

#define BUFSIZE 10000
#define MAX_LINE_LEN 1024
#define MSG_FLG "end"

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
      break;
    } else {
      // do nothing
    }

    // break the loop when the recieved data has "end"
    if (strcmp(buf, MSG_FLG) == 0) break;

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

void process_csv(int cl_sock)
{
  char buf[BUFSIZE];
  char replybuffer[10];
  int recv_size;

  while(1) {
    memset(buf, '\0', BUFSIZE);
    recv_size = recv(cl_sock, buf, BUFSIZE, 0);

    if (recv_size == -1) {
      perror("recv");
      exit(1);
    } else if (recv_size == 0) {
      break;
    } else  {
      // do nothing
    }

    if (strcmp(buf,"end") == 0) break;
  }
}

/* subst: c1とc2の文字を入れ替える */
int subst(char *str, char c1, char c2)
{
  int n = 0;

  while(*str){ //文字列が最後の'\0'になるまで繰り返す
      if(*str == c1){
  *str = c2;
    n++;
      }
      str++;
    }
  return n; //置き換えた数を返す
}

/*  get_line: １行読み込む */
int get_line(FILE *fp, char *line)
{
  if (fgets(line, MAX_LINE_LEN + 1, fp) == NULL){
    return 0;   /* failed or EOF */
  }
  subst(line, '\n', '\0');

  return 1;  /* succeeded */
}

int main(int argc, char* argv[])
{
  struct sockaddr_in addr; //dest address
  int cl_sock;  //socket descriptor
  struct hostent* hp;

  char buf[BUFSIZE]; // for data process
  int read_bufsize;

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
  addr.sin_port = htons(atoi(argv[2]));

  // make a connection
  if ((connect(cl_sock, (struct sockaddr *)&addr, sizeof(addr))) == -1) {
    perror("connect");
    return 1;
  }

  printf("================ Manual ================\n");
  printf("Input CSV data or command (Uppercase)\n");
  printf("%%Q   : Quit client\n");
  printf("%%C   : Check how many profiles you have\n");
  printf("%%P n : Print n profile(s)\n");
  printf("%%R file: Read file\n");
  printf("%%W file: Write data to file\n");
  printf("%%F word: Find word\n");
  printf("%%S n: Sort by nth column\n");
  printf("========================================\n");

  int csv_cnt = 0;
  while(1) {
    fflush(stdout);
    if (strchr(buf, ',') != NULL) {
      // print nothing
      ++csv_cnt;
    } else {
      printf("\nInput line:\n");
    }

    // get command input or csv data
    if (get_line(stdin, buf) == 0) {
      printf("error\n");
    }

    // send command line to the server from stdin
    if ((read_bufsize = send(cl_sock, buf, strlen(buf), 0)) == -1) {
      perror("send");
      return 1;
    }

    if (buf[0] == '%') {
      switch (buf[1]) {
        case 'P': process_print(cl_sock); break;
        case 'Q': process_else(cl_sock); break;
        case 'C': process_else(cl_sock); break;
        case 'R': process_else(cl_sock); break;
        case 'W': process_else(cl_sock); break;
        case 'F': process_print(cl_sock); break;
        case 'S': process_else(cl_sock); break;
      default:
        break;
      }
    } else {
      process_csv(cl_sock);
    }
  }
  close(cl_sock);
}
