//cp1.c
//2015115422 김진오
//2018-09-20 13:45

#include <stdio.h>
#include <unistd.h> //기본적 시스템 함수 사용을 위해
#include <fcntl.h>
#include <stdlib.h>
#define BUFFERSIZE 4096 //입력받을 공간 크기
#define COPYMODE   0644 //copymode 를 유니코드로 정해줌

void oops(char*,char*);
main(int ac, char *av[])
{
	int in_fd, out_fd, n_chars;
	char buf[BUFFERSIZE]; //입력받을 공간

	if(ac !=3){
		fprintf(stderr, "usage:%s source destination \n", *av);
		exit(1);
	} // 카피의 source 와 카피 할 file 이 입력되지 않으면 에러

	if ((in_fd=open(av[1], O_RDONLY))==-1) //카피source 파일 read로 연다
		oops("Cannot open" ,av[1]); //실패시 error 함수 실행
	if((out_fd=creat(av[2],COPYMODE))==-1) //카피할 파일 copymode로 연다
		oops("Cannot creat", av[2]);
	while((n_chars=read(in_fd, buf, BUFFERSIZE))>0)
			if(write(out_fd,buf,n_chars)!=n_chars)
			oops("Write error to", av[2]);
	// 버퍼사이즈단위로 파일 끝까지 읽고 만약 카피 할 파일에 맞지 않는
	// 용량이 들어 갈 경우 에러
	if(n_chars==-1)
		oops("read errorfrom",av[1]);

	if(close(in_fd)==-1||close(out_fd)==-1) //파일닫기
		oops("Error closing files", "");

}
void oops(char *s1, char *s2) //에러시 실행파일
{
	fprintf(stderr,"Error: %s ", s1);
	perror(s2);
	exit(1);

}


