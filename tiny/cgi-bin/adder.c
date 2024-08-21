/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE],visible[MAXBUF];
  int n1 = 0,n2 = 0;
//QUERY_STRING -> 15000&213
  if((buf=getenv("QUERY_STRING"))!=NULL){
    p = strchr(buf, '&');  //buf 문자열 에서 &를 찾으면 &의 포인터 반환
    *p = '\0'; //15000\0213
    //n1=1 에서 1만 남김
    strcpy(arg1, buf+3); //buf 주소를 arg1 주소로 복사
    //&n2=3 에서 3만 남김
    strcpy(arg2, p+4);
    n1 = atoi(arg1); //문자열을 정수로 변환
    n2 = atoi(arg2);
  }
  sprintf(content, "QUERY_STRING=%s", buf);
  // sprintf(content, "Welcome to add.com: ");
  // strcat(visible, content);
  // sprintf(content, "%sTHE Internet addition portal.\r\n<p>",content);
  // strcat(visible, content);
  sprintf(content, "%sTHE answer is: %d + %d = %d\r\n<p>",content,n1,n2,n1+n2);
  strcat(visible, content);
  // sprintf(content, "%sThanks for visiting!\r\n", content);
  // strcat(visible, content);

  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(visible));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s",visible);
  fflush(stdout);
  exit(0);
}
/* $end adder */
