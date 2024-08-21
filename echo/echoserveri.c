#include "csapp.h"  // CS:APP 교재의 커스텀 헤더 파일 포함
#include "echo.c"

void echo(int connfd);

int main(int argc, char **argv)
{
    int listenfd, connfd; //서버의 리스닝 소켓 파일 디스크립터, 클라이언트 연결 소켓 파일 디스크립터
    socklen_t clientlen;//클라이언트 주소 구조체의 길이
    struct sockaddr_storage clientaddr;//클라이언트 주소 정보를 저장할 구조체
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if(argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
    }

    listenfd = open_listenfd(argv[1]); //port 소켓 생성 완
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);//클라이언트 연결을 수락,클라이언트 식별자 받음
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE,
                    client_port, MAXLINE, 0);//연결된 클라이언트의 호스트명과 포트 정보를 가져옴.
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        echo(connfd);//echo 함수 호출하여 클라이언트와 통신
        Close(connfd);//통신끝나면 연결 닫음
    }

}