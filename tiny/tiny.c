/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize,char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;//서버의 리스닝 소켓 파일 디스크립터, 클라이언트 연결 소켓 파일 디스크립터
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
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    //클라이언트에서 넘겨받은 IP, port번호 정보 가져옴
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    //서버가 클라이언트의 connect요청을 받아들여 연결 확인 문구 출력
    printf("Accepted connection from (%s %s)\n", hostname, port);
  
    doit(connfd);
    Close(connfd);
  }
}
//연결된 소켓 connfd에서 정보를 받아 처리
//한개의 HTTP 트랜잭션을 처리하는 함수
void doit(int fd){
  int is_static = 0;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  //지정된 파일 디스크립터(fd)와 연결된 RIO 읽기 버퍼를 초기화
  Rio_readinitb(&rio, fd);
  //한줄읽기
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf); // "GET / HTTP/1.1"
  sscanf(buf, "%s %s %s", method, uri, version);
  //11.11숙제
  //무조건 GET 요청만 받음 POST 같은 다른 요청이면 즉시 return 후 연결소켓 파일디스크립터 닫음
  if(strcasecmp(method,"GET") && strcasecmp(method,"HEAD")){
    clienterror(fd, method, "501", "Not implemented","Tiny does not implement this method");
    return;
  }
  //GET 요청이면 읽는다
  read_requesthdrs(&rio);
  // printf("read request\n");
  //정적 파일인지 동적 파일인지 확인 정적파일 1 반환 , 동적파일 0 반환
  is_static = parse_uri(uri, filename, cgiargs);

  //stat 함수로 파일 상태 정보를 불러온다 성공 시 0 반환 실패시 -1 반환
  if(stat(filename,&sbuf)<0){
    //실패 시 에러메세지 띄움
    clienterror(fd, method, "404", "Not found","Tiny couldn't find this file");
    return;
  }
    // 정적 콘텐츠이면
  if(is_static==1){
    printf("is_static: %d\n", is_static);
    //파일이 일반파일이 아니거나 파일에 읽기 권한이 없거나 둘중 하나가 만족하면 에러 메세지 띄움
    //sbuf.st_mode =  파일 타입 및 권한
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR&sbuf.st_mode)){
      clienterror(fd, method, "403", "Forbidden","Tiny couldn't read the file");
      return;
    }
    //sbuf.st_size = 파일 크기(바이트)
    //클라이언트에게 정적 파일 제공
    serve_static(fd, filename, sbuf.st_size,method);
  }
  else{
    //동적 콘텐츠이면
    //파일이 일반파일이 아니거나 파일에 실행 권한이 없거나 둘중 하나가 만족하면 에러 메세지 띄움
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR&sbuf.st_mode)){
      clienterror(fd, method, "403", "Forbidden","Tiny couldn't read the file");
      return;
    }
    //클라이언트에게 동적 콘텐츠 파일 제공
    serve_dynamic(fd, filename, cgiargs);
  }
}
//HTTP 요청의 헤더 부분을 단순히 읽어 넘기는 역할
void read_requesthdrs(rio_t *rp){
  char buf[MAXLINE];
  //한줄읽기
  Rio_readlineb(rp, buf, MAXLINE);
  //buf가 개행문자에 닿기 전까지 while문 돈다
  while(strcmp(buf,"\r\n")){
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs){
  char *ptr;
  //정적 콘텐츠 파싱
  // strstr 이 NULL 이면(uri 문자 안의 cgi-bin 을 찾지 못헤 포인터 반환이 안된경우)
  // printf("in parse_uri");
  if(!strstr(uri,"cgi-bin")){
    //cgi 인자 스트링 지우고
    strcpy(cgiargs, "");
    //uri를 ./index.html 같은 상대 리눅스 경로 이름으로 변환
    strcpy(filename, ".");
    strcat(filename, uri);
    //만약 url 가 / 로 끝난다면
    if(uri[strlen(uri)-1]=='/'){
      strcat(filename, "home.html");
    }
    return 1;
  }//동적 콘텐츠 파싱
  else{
    ptr = index(uri, '?');
    if(ptr){
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else{
      strcpy(cgiargs, "");
    }
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}



void serve_static(int fd, char *filename, int filesize,char *method){
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF],header[MAXBUF]="";
  //filename 을 보고 filetype을 지정해줌
  get_filetype(filename, filetype);
  //클라이언트에게 응답보내기
  //클라이언트에 응답 줄과 응답 헤더 보내기 찐 데이터가 간게아니고 알림만 감
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  strcat(header, buf);
  sprintf(buf, "%sServer: Tiny web server\r\n", buf);
  strcat(header, buf);
  sprintf(buf, "%sConnction: close\r\n", buf);
  strcat(header, buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  strcat(header, buf);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  strcat(header, buf);
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s",header);
  //11.11 숙제
  if(strcasecmp(method,"HEAD")){
    //filename 포인터를 통해 읽기전용으로 파일 열기, 파일 디스크럽터 생성
    srcfd = Open(filename, O_RDONLY, 0);
    //숙제문제11.9
    //malloc을 사용하여 filesize만큼의 바이트 수 동적 할당
    srcp = (char *)malloc(filesize);
    //파일 내용을 할당된 메모리로 읽어옴
    Rio_readn(srcfd, srcp, filesize);
    //메모리에 있는 파일 내용을 클라이언트에게 전송
    Rio_writen(fd, srcp, filesize);
    //전송 완료되면 메모리 할당 해제
    Free(srcp);
    //파일도 닫음
    Close(srcfd);
    //서버 가상 메모리 주소 0 에 filesize바이트 매핑, 말록과비슷한개념
    // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    // //메모리 할당 완료 했으니 파일 디스크립터 닫음
    // Close(srcfd);
    // printf("close file\n");
    // //클라이언트에 파일 전송
    // Rio_writen(fd, srcp, filesize);
    // printf("sent file\n");
    // //전송 완료 했으니 가상메모리 해제
    // Munmap(srcp, filesize);
    }
}


//정적파일 타입별로 filetype을 정해줌 5가지 타입.
//5가지 타입을 벗어나는 경우는 모두 text/plain으로
void get_filetype(char *filename, char *filetype){
  //filename 에 .html 이라는 문자열이 포함되어 있으면
  if(strstr(filename,".html")){
      // 비어있는 filetype 인자에 text/html 문자열을 넣음
      strcpy(filetype, "text/html");
  }
  else if (strstr(filename,".gif")){
      strcpy(filetype, "image/gif");
  }
  else if (strstr(filename,".png")){
      strcpy(filetype, "image/png");
  }
  else if (strstr(filename,".jpg")){
      strcpy(filetype, "image/jpeg");
  }
  else if (strstr(filename,".mp4")){
      strcpy(filetype, "video/mp4");
  }
  else{
      strcpy(filetype, "text/plain");
  }
}

void serve_dynamic(int fd, char *filename, char *cgiargs){
  char buf[MAXLINE], *emptylist[] = {NULL};
  //HTTP/1.0 200 OK를 버퍼에 넣고
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  //클라이언트에 버퍼 전송
  Rio_writen(fd, buf, strlen(buf));
  //Server: Tiny Web Serve   버퍼에 넣고
  sprintf(buf, "Server: Tiny Web Server\r\n");
  //클라이언트에 버퍼 전송
  Rio_writen(fd, buf, strlen(buf));

  if(Fork()==0){
    //환경변수 QUERY_STRING에 cgiargs 값 할당 , 하위 프로세스에 정보 전달
    setenv("QUERY_STRING", cgiargs, 1);
    //표준 입출력 리다이렉션
    Dup2(fd, STDOUT_FILENO);
    //현재 프로세스를 새로운 프로그램으로 대체
    Execve(filename, emptylist, environ);
  }
  Wait(NULL);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];
  // 에러 틀 에 인자 넣어 내용 추가해서 body에 넣음
  // 길어서 나눠서 body로 보냄
  sprintf(body, "<html><title>Tiny Error</html>");
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