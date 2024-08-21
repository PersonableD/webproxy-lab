#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int client_fd);
int parse_uri(char *uri, char *host, char *port, char *path);
void forward_response(int server_fd, int client_fd);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);


int main(int argc, char **argv) {
 int listenfd, clientfd;//서버의 리스닝 소켓 파일 디스크립터, 클라이언트 연결 소켓 파일 디스크립터
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;//클라이언트 주소 구조체의 길이
  struct sockaddr_storage clientaddr;//클라이언트 주소 정보를 저장할 구조체

  if(argc !=2){
    //argv[0] 에는 일반적으로 프로그램의 이름이 들어감
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  listenfd = Open_listenfd(argv[1]); //포트번호 포인터를 넣어 요청 받을 준비된 소켓생성
  while(1){
    //구조체 크기를 clientlen에 담음
    clientlen = sizeof(clientaddr);
    //새로운 연결된 소켓 connfd 생성
    clientfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    //클라이언트에서 넘겨받은 IP, port번호 정보 가져옴
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    //서버가 클라이언트의 connect요청을 받아들여 연결 확인 문구 출력
    printf("Accepted connection from (%s %s)\n", hostname, port);
  
    doit(clientfd);
    Close(clientfd);
  }
  return 0;
}

void doit(int client_fd){
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE],version[MAXLINE];
  char hostname[MAXLINE], port[MAXLINE], path[MAXLINE];
  rio_t rio;

  //지정된 파일 디스크립터(fd)와 연결된 RIO 읽기 버퍼를 초기화
  Rio_readinitb(&rio, client_fd);
  //한줄읽기
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf); // "GET / HTTP/1.1"
  // GET http://www.cmu.edu/hub/index.html HTTP/1.1
  sscanf(buf, "%s %s %s", method, uri,version);
  //HEAD 메서드 처리 추가
  if(strcasecmp(method,"GET") && strcasecmp(method,"HEAD")){
    clienterror(client_fd, method, "501", "Not implemented","Tiny does not implement this method");
    return;
  }
  // GET http://www.cmu.edu/hub/index.html HTTP/1.1
  //hostname, port, path 에 파싱한 값이 들어옴
  // hostname =www.cmu.edu
  // port = 80
  // path = /hub/index.html
  parse_uri(uri, hostname, port, path);
  //새로운 소켓 생성
  int server_fd = Open_clientfd(hostname, port);
  if(server_fd<0){
    clienterror(client_fd, method, "502", "Bad Gateway", "Failed to connect to server");
  }
  //서버로 요청 전달
  char server_request[MAXLINE];
  //server_request에  method, path, version 를 담음 
  //GET /hub/index.html 1.1
  sprintf(server_request, "%s %s %s\r\n", method,path,version);
  Rio_writen(server_fd, server_request, strlen(server_request));

  // 나머지 헤더 전달
  Rio_readlineb(&rio, buf, MAXLINE);
  while(strcmp(buf,"\r\n")){
    Rio_writen(server_fd, buf, strlen(buf));
    Rio_readlineb(&rio, buf, MAXLINE);
  }
  //추가적으로 \r\n도 보냄 줄바꿈을 위해
  Rio_writen(server_fd, "\r\n", 2);

  forward_response(server_fd, client_fd);

  Close(server_fd);
}

int parse_uri(char *uri, char *host, char *port, char *path) {
    char *host_start, *port_start, *path_start;

    // "http://" 제거
    host_start = strstr(uri, "//");
    host_start = host_start ? host_start + 2 : uri;

    // 경로 찾기
    path_start = strchr(host_start, '/');
    if (path_start) {
        strcpy(path, path_start);
        *path_start = '\0';  // 호스트 문자열 종료
    } else {
        strcpy(path, "/");
    }

    // 포트 찾기
    port_start = strchr(host_start, ':');
    if (port_start) {
        *port_start = '\0';  // 호스트 문자열 종료
        strcpy(port, port_start + 1);
    } else {
        strcpy(port, "80");
    }

    // 호스트 복사
    strcpy(host, host_start);

    return 0;
}

//파싱 함수
// int parse_uri(char *uri, char *host, char *port, char *path) {
//     char *proto_end, *host_start, *host_end, *port_start, *path_start;

//     // "http://" 제거
//     proto_end = strstr(uri, "//");
//     if (proto_end) {
//         host_start = proto_end + 2;
//     } else {
//         host_start = uri;
//     }

//     // 경로 시작점 찾기
//     path_start = strchr(host_start, '/');
//     if (path_start) {
//         host_end = path_start;
//         strcpy(path, path_start);  // '/'를 포함하여 복사
//     } else {
//         host_end = host_start + strlen(host_start);
//         strcpy(path, "/");  // 경로가 없으면 "/"만 저장
//     }

//     // 포트 번호 찾기
//     port_start = strchr(host_start, ':');
//     if (port_start && port_start < host_end) {
//         // 포트 번호가 있는 경우
//         strncpy(host, host_start, port_start - host_start);
//         host[port_start - host_start] = '\0';
//         strncpy(port, port_start + 1, host_end - port_start - 1);
//         port[host_end - port_start - 1] = '\0';
//     } else {
//         // 포트 번호가 없는 경우
//         strncpy(host, host_start, host_end - host_start);
//         host[host_end - host_start] = '\0';
//         strcpy(port, "80");  // 기본 HTTP 포트
//     }

//     return 0;
// }



void forward_response(int server_fd,int client_fd)
{

    char buf[MAXBUF];
    ssize_t n;

    while ((n = Read(server_fd, buf, MAXBUF)) > 0) {
        printf("Proxy received %d bytes, then send\n", (int)n);
        Rio_writen(client_fd, buf, n);
    }

    if (n < 0) {
        fprintf(stderr, "Error reading from server\n");
    }
    // char buf[MAXLINE];//데이터를 읽고 쓰는 데 사용할 버퍼
    // size_t n;//읽은 바이트 수를 저장할 변수
    // rio_t server_rio;
    // Rio_readinitb(&server_rio, server_fd);
    // //반환값 n= 실제로 읽은 바이트 수
    // while((n=Rio_readlineb(&server_rio,buf,MAXLINE))!=0){
    //     printf("Proxy server received %d bytes,then send\n", (int)n);//서버가 받은 데이터의 바이트 수 출력
    //     //쓴다 = 보낸다!
    //     Rio_writen(client_fd, buf, n); //읽은 데이터를 그대로 클라이언트에게 다시 보냄
    // }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];
  // 에러 틀 에 인자 넣어 내용 추가해서 body에 넣음
  // 길어서 나눠서 body로 보냄
  sprintf(body, "<html><title>Proxy Error</html>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n",body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>Tiny Web server</em>\r\n", body);

  //한줄씩 클라이언트로 보냄
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n",(int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  //바디내용도 클라이언트로 보냄
  Rio_writen(fd, body, strlen(body));

}