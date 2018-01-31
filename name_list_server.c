/********************************************
name_list_server:
 クライアントから送られたコマンドを受け取り，
 処理結果をクライアントに返す
********************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>

#define BUFSIZE 10000
#define MAX_LINE_LEN 1024
#define MAX_STR_LEN 69
#define MSG_FLG "end"

char sendbuf[BUFSIZE];
int profile_data_nitems = 0; //CSVデータの数
void parse_line(char *line, int sock);

void endmsg(int new_sock)
{
  char replybuffer[10];
  // データ送信終了を意味する"end"を送る
  memset(replybuffer, 0, sizeof(replybuffer));
  sprintf(replybuffer, MSG_FLG);
  send(new_sock, replybuffer, sizeof(replybuffer),0);
  return;
}

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

struct profile profile_data_store[10000];

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

/* split: CSVデータを分割する */
int split(char *str, char *ret[], char sep, int max) //sep: ','
{
  int cnt = 0;

  ret[cnt] = str;
  cnt = cnt + 1;

  while(*str && cnt < max) { //文字列が最後の'\0'になるまでかつ分割数がmaxを越えるまで繰り返す
    if(*str == sep){
      *str = '\0';
      ret[cnt] = str + 1;
      cnt++;
    }
    str++;

  }
  return cnt;
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

/* date_to_string */
void *date_to_string(char buf[], struct date *date)
{
  sprintf(buf, "%04d-%02d-%02d", date->y, date->m, date->d);

  return buf;
}

/*  fprint_profile_csv: %Wコマンドを利用したとき，書き出すデータを表示する */
void fprint_profile_csv(FILE *fp, struct profile *p)
{
  fprintf(fp, "%d,%s,%d-%d-%d,%s,%s\n", p->number, p->name, p->birthday.y,
p->birthday.m, p->birthday.d, p->address, p->comment );
}

/* print_profile: 5つの情報を出力する */
void print_profile(struct profile *p)
{
  char date[11];

  printf("number:   %d\n", p->number);
  printf("name:     %s\n", p->name);
  printf("birthday: %d-%d-%d\n", p->birthday.y, p->birthday.m, p->birthday.d);
  printf("address:  %s\n", p->address);
  printf("comment:  %s\n", p->comment);

}

/* swap_profile: p1とp2を入れ替える */
void swap_profile(struct profile *p1, struct profile *p2)
{
  struct profile tmp;

  tmp = *p2;
  *p2 = *p1;
  *p1 = tmp;
}

/* Date compare function conforming to qsort */
int compare_date(struct date *d1, struct date *d2)
{
  if (d1->y != d2->y) return d1->y - d2->y;
  if (d1->m != d2->m) return d1->m - d2->m;
  return d1->d - d2->d;
}

/* Profile compare functions conforming to qsort
   1: s1 > s2
   0: s1 = s2
   -1 < s2
*/

int compare_number(struct profile *p1, struct profile *p2)
{
  return p1->number - p2->number;
}

int compare_name(struct profile *p1, struct profile *p2)
{
  return strcmp(p1->name, p2->name);
}

int compare_birthday(struct profile *p1, struct profile *p2)
{
  return compare_date(&p1->birthday, &p2->birthday);
}

int compare_address(struct profile *p1, struct profile *p2)
{
  return strcmp(p1->address, p2->address);
}

int compare_comment(struct profile *p1, struct profile *p2)
{
  return strcmp(p1->comment, p2->comment);
}

int (*compare_functions[])(struct profile *p1, struct profile *p2) = {

    compare_number,
    compare_name,
    compare_birthday,
    compare_address,
    compare_comment
 };

/* quick sort */
void q_sort(struct profile p[], int left, int right, int (*compare_func)(struct profile *p1, struct profile *p2))
{
  struct profile pivot = p[(left + right) / 2];
  int l = left, r = right;

  if(!(left < right)) return;

  while(l <= r) {
    while(compare_func(&pivot, &p[l]) > 0) l++;
    while(compare_func(&pivot, &p[r]) < 0) r--;

    if(l <= r) {
      swap_profile(&p[l++], &p[r--]);
    }
  }
  q_sort(p, left, r, compare_func);
  q_sort(p, l, right, compare_func);
}

int max(int a, int b)
{
  if(a > b) return a;
  return b;
}

/*********************************************************
          profile manipulation
 ********************************************************/

struct date *new_date(struct date *d, char *str)
{
  char *ptr[3];

  if(split(str, ptr, '-', 3) != 3)
    return NULL;

  d->y = atoi(ptr[0]);
  d->m = atoi(ptr[1]);
  d->d = atoi(ptr[2]);

  return d;
}

struct profile *new_profile(struct profile *p, char *csv)
{
  char *ptr[5];

  if(split(csv, ptr, ',', 5 ) != 5)
   return NULL;

  /*  ID: number */
  p->number = atoi(ptr[0]);

  /* 学校名: name */
  strncpy(p->name, ptr[1], MAX_STR_LEN);
  p->name[MAX_STR_LEN] = '\0';

  /* 設立年月日: birthday */
  if(new_date(&p->birthday, ptr[2]) == NULL)
    return NULL;

  /* 所在地: address */
  strncpy(p->address, ptr[3], MAX_STR_LEN);
  p->address[MAX_STR_LEN] = '\0';

  /* 備考データ: comment */
  p->comment = (char*)malloc(sizeof(char) * (strlen(ptr[4])+1));
  strcpy(p->comment, ptr[4]);

  return p;
}

/********************************************************
            command
 ********************************************************/

/**
 * Command %Q
 */
void cmd_quit()
{
  return;
}

/**
 * Command %C
 */
void cmd_check()
{
  memset(sendbuf, '\0', BUFSIZE);
  sprintf(sendbuf, "%d profile(s)\n", profile_data_nitems);
}

/**
 * Command %P count
 */
void cmd_print(int nitems, int sock)
{
  int i;
  struct profile *p;
  p=&profile_data_store[0];
  int send_size;
  char printbuf[1024];

  if (nitems == 0) {
    for(i=0; i<profile_data_nitems; i++) {
      p=&profile_data_store[i];

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "number:  %d\n", p->number);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "name:  %s\n", p->name);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "birthday:  %d-%d-%d\n", p->birthday.y, p->birthday.m, p->birthday.d);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "address:  %s\n", p->address);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "comment:  %s\n\n", p->comment);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }
      print_profile(p);
      printf("\n");
    }
  }

  /* n > 0: n件分表示  */
  if(nitems > 0){
    for(i = 0; i < nitems; i++) {
      p=&profile_data_store[i];

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "number:  %d\n", p->number);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "name:  %s\n", p->name);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "birthday:  %d-%d-%d\n", p->birthday.y, p->birthday.m, p->birthday.d);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "address:  %s\n", p->address);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "comment:  %s\n\n", p->comment);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }
      print_profile(p);
      printf("\n");
    }
  }

  /* n < 0: 後ろから-n件表示 */
  if(nitems < 0){
    i = profile_data_nitems + nitems;
    nitems = profile_data_nitems;
    for(; i < nitems; i++) {
      p=&profile_data_store[i];

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "number:  %d\n", p->number);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "name:  %s\n", p->name);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "birthday:  %d-%d-%d\n", p->birthday.y, p->birthday.m, p->birthday.d);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "address:  %s\n", p->address);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }

      memset(printbuf, '\0', sizeof(printbuf));
      sprintf(printbuf, "comment:  %s\n\n", p->comment);
      if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
        perror("send");
        exit(1);
      }
      print_profile(p);
      printf("\n");
    }
  }
}

/**
 * Command %R filename
 */
void cmd_read(char *filename, int sock)
{
  FILE *fp;
  char line[MAX_LINE_LEN + 1];

  memset(sendbuf, '\0', BUFSIZE);

  if ((fp = fopen(filename, "r")) == NULL) {
    sprintf(sendbuf, "can't open the file\n");
    exit(1);
  } else {
    sprintf(sendbuf, "Filename: %s\n", filename);
    //sprintf(sendbuf, "end");
  }

  while (get_line(fp, line)) {
    parse_line(line, sock);
  }

  fclose(fp);
}

/**
 * Command %W filename
 */
void cmd_write(char *filename)
{
  FILE *fp;
  int i;

  memset(sendbuf, '\0', BUFSIZE);

  if ((fp = fopen(filename, "w")) == NULL) {
    sprintf(sendbuf, "can't open the file\n");
    exit(1);
  } else {
    sprintf(sendbuf, "Filename: %s\n", filename);
  }

  for (i=0; i<profile_data_nitems; i++) {
    fprint_profile_csv(fp, &profile_data_store[i]);
  }
  fclose(fp);
}

/**
 * Command %F word
 */
void cmd_find(char *word, int sock)
{
  int i;
  struct profile *p;
  char number_str[8];
  char birthday_str[11];
  int send_size;
  char printbuf[1024];

  memset(sendbuf, '\0', BUFSIZE);
  for (i=0; i<profile_data_nitems; i++) {
    p = &profile_data_store[i];

    date_to_string(birthday_str, &p->birthday);
    sprintf(number_str, "%d", p->number);

    if (strcmp(number_str, word) == 0   ||
        strcmp(p->name, word) == 0      ||
        strcmp(birthday_str, word) == 0 ||
        strcmp(p->address, word) == 0   ||
        strcmp(p->comment, word) == 0) {

          memset(printbuf, '\0', sizeof(printbuf));
          sprintf(printbuf, "number:  %d\n", p->number);
          if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
            perror("send");
            exit(1);
          }

          memset(printbuf, '\0', sizeof(printbuf));
          sprintf(printbuf, "name:  %s\n", p->name);
          if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
            perror("send");
            exit(1);
          }

          memset(printbuf, '\0', sizeof(printbuf));
          sprintf(printbuf, "birthday:  %s\n", birthday_str);
          if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
            perror("send");
            exit(1);
          }

          memset(printbuf, '\0', sizeof(printbuf));
          sprintf(printbuf, "address:  %s\n", p->address);
          if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
            perror("send");
            exit(1);
          }

          memset(printbuf, '\0', sizeof(printbuf));
          sprintf(printbuf, "comment:  %s\n\n", p->comment);
          if ((send_size = send(sock, printbuf, sizeof(printbuf), 0)) == -1 ) {
            perror("send");
            exit(1);
          }
    }
  }
}

/**
 * Command %S column
 */
void cmd_sort(int column)
{
  int abs_col = max(column, -column);

  memset(sendbuf, '\0', BUFSIZE);

  if(abs_col > 5 || abs_col < 1) {
    sprintf(sendbuf, "invalid column number\n");
    return;
  }
  if(column > 0) {
    q_sort(profile_data_store, 0, profile_data_nitems-1, compare_functions[abs_col-1]);
    sprintf(sendbuf, "Sorted by column %d\n", column);
  }
}

/************************************************************
        command dispatcher
 ************************************************************/
void exec_command(char cmd, char *param, int sock)
{
  int send_size;

  switch (cmd) {
    case 'Q': cmd_quit();              break;   //done
    case 'C': cmd_check();             break;   //done
    case 'R': cmd_read(param, sock);         break;   //done
    case 'W': cmd_write(param);        break;   //done
    case 'S': cmd_sort(atoi(param));   break;
    default:
      break;
  }
}

/* parse_line: get_lineで得られた入力行lineの1文字目を見て'%'かどうかを判断する */
void parse_line(char *line, int sock)
{
  if (line[0] == '%') { //コマンド入力として処理
    exec_command(line[1], &line[3], sock);
  } else if ( line[0] == '\n' || line[0] == '\0') {
    // do nothing
  } else if (strchr(line, ',') != NULL){ //CSVの行を処理
    new_profile(&profile_data_store[profile_data_nitems++], line);
  }
}

int main(int argc, char** argv)
{
  struct sockaddr_in addr;
  int s_sock;
  int new_sock;
  int cl_addr_len;
  int recv_bufsize;
  char buf[BUFSIZE];

  // 1. Create socket
  s_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (s_sock == -1) {
    perror("socket");
    exit(1);
  }

  memset((char*)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(atoi(argv[1]));

  printf("1. Created socket\n");

  // 2. Name socket
  if (bind(s_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind");
    exit(1);
  }
  printf("2. Named socket\n");

  // 3. Listen connection request
 LABEL1:
  if (listen(s_sock, 5) == -1) {
    perror("listen");
    exit(1);
  }
  printf("3. listening connection request\n");

  // 4. Accept connection request
  memset(sendbuf, '\0', BUFSIZE);
  cl_addr_len = sizeof(addr);

  while(1) {
    if ((new_sock = accept(s_sock, (struct sockaddr*)&addr, &cl_addr_len)) == -1) {
      perror("accept");
      exit(1);
    }
    printf("4. Accepted connection request\n");

    // 5. Recieve command
    while(1) {
      memset(buf, '\0', BUFSIZE);
      if ((recv_bufsize = recv(new_sock, buf, BUFSIZE, 0)) == -1) {
        perror("recv");
        exit(1);
      } else if (recv_bufsize == 0) break;
      else {
        // do nothing
      }

      printf("5. Received msg\n");
      printf("%s\n", buf);

      // 6. Process the command
      if (buf[1] == 'Q') {
        memset(sendbuf, '\0', BUFSIZE);
        sprintf(sendbuf, "exit");

        if ((recv_bufsize = send(new_sock, sendbuf, BUFSIZE, 0)) == -1) {
          perror("send");
          exit(1);
        }
        goto LABEL1; // go to listen(LABEL1) and quit current client connection

      } else if (buf[1] == 'P') {
        cmd_print(atoi(&buf[3]), new_sock);
        sleep(1);
        endmsg(new_sock);

      } else if (buf[1] == 'F') {
        cmd_find(&buf[3], new_sock);
        sleep(1);
        endmsg(new_sock);

      } else if (strchr(buf, ',') != NULL) {
        parse_line(buf, new_sock);
        memset(buf, '\0', BUFSIZE);
        endmsg(new_sock);

      } else {
        // parse command except %Q, %P & %F
        parse_line(buf, new_sock);
        memset(buf, '\0', BUFSIZE);

        // 7. Send results
        if ((recv_bufsize = send(new_sock, sendbuf, BUFSIZE, 0)) == -1) {
          perror("send");
          exit(1);
        }
      }
    }
    close(new_sock);
  }
  close(s_sock);
}
