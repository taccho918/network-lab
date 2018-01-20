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

#define PORT_NO 12225
#define BUFSIZE 10000
#define MAX_LINE_LEN 1024
#define MAX_STR_LEN 69

char sendbuf[BUFSIZE];
int profile_data_nitems = 0; //CSVデータの数
void parse_line(char *line);

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

/* trim_ncode: 改行コードの削除 */
void trim_ncode(char *str) 
{
  int i = 0;
  while(1) {
    if(str[i] == '\n') {
      str[i] = '\0';
      break;
    }
    i++;
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

/*  fprint_profile_csv: %Wコマンドを利用したとき，書き出すデータを表示する */
void fprint_profile_csv(FILE *fp, struct profile *p)
{
  fprintf(fp, "%d,%s,%d-%d-%d,%s,%s\n", p->number, p->name, p->birthday.y, 
p->birthday.m, p->birthday.d, p->address, p->comment );
  printf("%d,%s,%d-%d-%d,%s,%s\n", p->number, p->name, p->birthday.y, 
p->birthday.m, p->birthday.d, p->address, p->comment );
}

/* date_to_string */
void *date_to_string(char buf[], struct date *date)
{
  sprintf(buf, "%04-%02-%02", date->y, date->m, date->d);
 
  return buf;
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
void cmd_print(int nitems)
{
  int i = 0;
  struct profile *p;
  p = &profile_data_store[0];

  memset(sendbuf, '\0', BUFSIZE);
  /*  n = 0: 全件表示 */
  if(nitems == 0){  
    for (i = 0; i < profile_data_nitems; i++) { 
      print_profile(&profile_data_store[i]);
      printf("\n");
    }
  }
  /* n > 0: n件分表示  */
  if(nitems > 0){
    for(i = 0; i < nitems; i++) {
      print_profile(&profile_data_store[i]);
      printf("\n");
    }
  }
  /* n < 0: 後ろから-n件表示 */
  if(nitems < 0){
    i = profile_data_nitems + nitems;
    nitems = profile_data_nitems;
    for(; i < nitems; i++) {
      print_profile(&profile_data_store[i]);
      printf("\n");
    }
  }
}

/**
 * Command %R filename
 */
void cmd_read(char *filename)
{
  FILE *fp;
  char line[MAX_LINE_LEN + 1];
  
  trim_ncode(filename);
  memset(sendbuf, '\0', BUFSIZE);
  
  if ((fp = fopen(filename, "r")) == NULL) {
    sprintf(sendbuf, "can't open the file\n");
    sprintf(sendbuf, "%s\n", strerror(errno));
    exit(1);
  } else {
    sprintf(sendbuf, "Filename: %s\n", filename);
  }

  while (get_line(fp, line)) {
    parse_line(line);
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

  trim_ncode(filename);
  memset(sendbuf, '\0', BUFSIZE);

  if ((fp = fopen(filename, "w")) == NULL) {
    sprintf(sendbuf, "can't open the file\n");
    sprintf(sendbuf, "%s\n", strerror(errno));
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
void cmd_find(char *word)
{
  int i;
  struct profile *p;
  char number_str[8];
  char birthday_str[11];

  memset(sendbuf, '\0', BUFSIZE);
  trim_ncode(word);
  for (i=0; i<profile_data_nitems; i++) {
    p = &profile_data_store[i];

    date_to_string(birthday_str, &p->birthday);
    sprintf(number_str, "%d", p->number);

    if (strcmp(number_str, word) == 0   ||
        strcmp(p->name, word) == 0      || 
        strcmp(birthday_str, word) == 0 ||      
        strcmp(p->address, word) == 0   ||
        strcmp(p->comment, word) == 0) {
      print_profile(p);
      printf("\n");
    } else {
      sprintf(sendbuf, "no match\n");
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
    sprintf(sendbuf, "invalid column number");
    return;
  }
  if(column > 0) {
    q_sort(profile_data_store, 0, profile_data_nitems-1, compare_functions[abs_col-1]);
  }
}

/**
 * Command %D column 
 */
void cmd_delete()
{
  profile_data_nitems = 0;

  fprintf(stdin, "%d profiles", profile_data_nitems);
  printf("All of profiles were deleted.\n");
}


/************************************************************
        command dispatcher
 ************************************************************/
void exec_command(char cmd, char *param)
{ 
  switch (cmd) {
    case 'Q': cmd_quit();              break;   //done
    case 'C': cmd_check();             break;   //done
    case 'P': cmd_print(atoi(param));  break; 
    case 'R': cmd_read(param);         break;   //done
    case 'W': cmd_write(param);        break;   //done
    case 'F': cmd_find(param);         break;   //done
    case 'S': cmd_sort(atoi(param));   break;   //done
    case 'D': cmd_delete();            break;
    default:
      sprintf(sendbuf, "%%c command is not existed: ignored\n", cmd);
      break;
  }
}

/* parse_line: get_lineで得られた入力行lineの1文字目を見て'%'かどうかを判断する */
void parse_line(char *line)
{
  if (line[0] == '%') { //コマンド入力として処理
    exec_command(line[1], &line[3]);  
  } else { //CSVの行を処理
    new_profile(&profile_data_store[profile_data_nitems], line);
    profile_data_nitems++;
  }
}

int main(int argc, char** argv)
{
  struct sockaddr_in addr;
  int s_sock;

  // 1. Create socket
  s_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (s_sock == -1) {
    printf("error: socket\n");
    return (1);
  }

  memset((char*)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(PORT_NO);
  
  printf("1. Created socket\n");

  // 2. Name socket 
  if (bind(s_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    printf("error: bind\n");
    return (1);
  }
  printf("2. Named socket\n");

  // 3. Listen connection request
 LABEL1:
  if (listen(s_sock, 5) == -1) {
    printf("error: listen\n");
    return (1);
  }
  printf("3. listening connection request\n");

  // 4. Accept connection request
  int new_sock;
  int cl_addr_len;
  int recv_bufsize;
  int send_bufsize;
  char buf[BUFSIZE];
 
  memset(sendbuf, '\0', BUFSIZE);

  cl_addr_len = sizeof(addr);

  while(1) {
    new_sock = accept(s_sock, (struct sockaddr*)&addr, &cl_addr_len);
    if (new_sock == -1) {
      printf("error: accept\n");
      return (1);
    }
    printf("4. Accepted connection request\n");

    // 5. Recieve command
    while(1) {
      memset(buf, '\0', BUFSIZE);
      recv_bufsize = recv(new_sock, buf, BUFSIZE, 0);
      if (recv_bufsize == -1) {
        printf("error: recv\n");
        return 1;
      } else if (recv_bufsize == 0) break;

      printf("5. Received command\n");
      printf("%s\n", buf);

      // 6. Process the command
      if (strcmp(buf, "%Q\n") == 0) {
        memset(sendbuf, '\0', BUFSIZE);
        sprintf(sendbuf, "exit");
        memcpy(buf, sendbuf, BUFSIZE);
        send_bufsize = send(new_sock, buf, BUFSIZE, 0);
        if (send_bufsize == -1) {
          printf("error: send\n");
          return 1;
        }
        goto LABEL1; // go to listen(LABEL1) and quit current client connection
      }
      else {
        parse_line(buf);
      }

      memset(buf, '\0', BUFSIZE);
      memcpy(buf, sendbuf, BUFSIZE);

      // 7. Send results
      send_bufsize = send(new_sock, buf, BUFSIZE, 0);
      if (send_bufsize == -1) {
        printf("error: send\n");
        return 1;
      }
      printf("6. Sent results\n");
    }
    close(new_sock);
    printf("closed new socket\n");
  }
  close(s_sock);
}
