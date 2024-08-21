#include "csapp.h"  // CS:APP 교재의 커스텀 헤더 파일 포함

int main(int argc, char **argv)
{
    int clientfd;  // 클라이언트 소켓의 파일 디스크립터
    char *host, *port, buf[MAXLINE];  // 호스트, 포트, 입출력 버퍼 선언
    rio_t rio;  // Robust I/O (RIO) 구조체 선언

    if (argc != 3) {  // 명령줄 인자가 3개가 아니면 (프로그램 이름, 호스트, 포트)
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);  // 사용법 출력
        exit(0);  // 프로그램 종료
    }
    host = argv[1];  // 첫 번째 인자를 호스트로 설정
    port = argv[2];  // 두 번째 인자를 포트로 설정

    clientfd = Open_clientfd(host, port);  // 서버에 연결, 소켓 생성
    Rio_readinitb(&rio, clientfd);  // RIO 읽기 버퍼 초기화

    while (Fgets(buf, MAXLINE, stdin) != NULL) {  // 표준 입력에서 한 줄 읽기
        Rio_writen(clientfd, buf, strlen(buf));  // 읽은 내용을 서버로 전송
        Rio_readlineb(&rio, buf, MAXLINE);  // 서버로부터 응답 읽기
        Fputs(buf, stdout);  // 서버의 응답을 표준 출력에 출력
    }
    Close(clientfd);  // 클라이언트 소켓 닫기
    exit(0);  // 프로그램 정상 종료
}