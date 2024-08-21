#include "csapp.h"  // CS:APP 교재의 커스텀 헤더 파일 포함

void echo(int connfd)
{
    size_t n;//읽은 바이트 수를 저장할 변수
    char buf[MAXLINE];//데이터를 읽고 쓰는 데 사용할 버퍼
    rio_t rio;
    Rio_readinitb(&rio, connfd);
    //반환값 n= 실제로 읽은 바이트 수
    while((n=Rio_readlineb(&rio,buf,MAXLINE))!=0){
        printf("server received %d bytes\n", (int)n);//서버가 받은 데이터의 바이트 수 출력
        //쓴다 = 보낸다!
        Rio_writen(connfd, buf, n); //읽은 데이터를 그대로 클라이언트에게 다시 보냄
    }
}