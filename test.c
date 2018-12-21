#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

#define MAXLINE  511
#define MAX_SOCK 1024 // 솔라리스의 경우 64
#define quiz 5
char *EXIT_STRING = "exit";	// 클라이언트의 종료요청 문자열
char *START_STRING = "Connected to server \n";
// 클라이언트 환영 메시지
int maxfdp1;				// 최대 소켓번호 +1
int num_user = 0;			// 채팅 참가자 수
int num_chat = 0;			// 지금까지 오간 대화의 수
int clisock_list[MAX_SOCK];		// 채팅에 참가자 소켓번호 목록
char ip_list[MAX_SOCK][20];
char user_list[MAX_SOCK][20];		//접속한 ip목록
int listen_sock;			// 서버의 리슨 소켓

char win[20];
int game_flag=0;
int nation_num=0;
int nation_name_flag[quiz]={0,};	
				
char nation[quiz][100]={{"korea"},{"japan"},{"England"},{"China"},{"vetnam"}};
char city[quiz][100]={{">seoul"},{">tokyo"},{">london"},{">beijing"},{">hanoi"}}; 
void winner(char *);
void game_start();// 게임진행
void addClient(int s, struct sockaddr_in *newcliaddr);// 새로운 채팅 참가자 처리
int getmax();				// 최대 소켓 번호 찾기
void removeClient(int s);	// 채팅 탈퇴 처리 함수
int tcp_listen(int host, int port, int backlog); // 소켓 생성 및 listen
int set_ticker(int n_msecs);
void handle(int signum);
void *thread_function(void *arg);
void findwinner();
void errquit(char *mesg) { perror(mesg); exit(1); }

time_t ct;
struct tm tm;


int main(int argc, char *argv[]) {
	struct sockaddr_in cliaddr;
	char buf[MAXLINE + 1]; //클라이언트에서 받은 메시지
	char message[100];
	int i, j,k, nbyte, accp_sock, addrlen = sizeof(struct
		sockaddr_in);
	int flag;
	fd_set read_fds;	//읽기를 감지할 fd_set 구조체
	pthread_t a_thread;

	
	void handle(int);
	if (argc != 2) {
		printf("How to use :%s port\n", argv[0]);
		exit(0);
	}

	//signal(SIGALRM,handle);
	//set_ticker(500);

  	// tcp_listen(host, port, backlog) 함수 호출
	listen_sock = tcp_listen(INADDR_ANY, atoi(argv[1]), 5);
	//스레드 생성
	pthread_create(&a_thread, NULL, thread_function, (void *)NULL);
	while (1) {
		FD_ZERO(&read_fds);
		FD_SET(listen_sock, &read_fds);
		for (i = 0; i < num_user; i++)
			FD_SET(clisock_list[i], &read_fds);

		maxfdp1 = getmax() + 1;	// maxfdp1 재 계산
		if (select(maxfdp1, &read_fds, NULL, NULL, NULL) < 0)
			errquit("select fail");

		if (FD_ISSET(listen_sock, &read_fds)) {
			accp_sock = accept(listen_sock,
				(struct sockaddr*)&cliaddr, &addrlen);
			if (accp_sock == -1) errquit("accept fail");
			addClient(accp_sock, &cliaddr);
			send(accp_sock, START_STRING, strlen(START_STRING), 0);
			ct = time(NULL);			//현재 시간을 받아옴
			tm = *localtime(&ct);
			write(1, "\033[0G", 4);		//커서의 X좌표를 0으로 이동
			printf("[%02d:%02d:%02d]", tm.tm_hour, tm.tm_min, tm.tm_sec);
			fprintf(stderr, "\033[33m");//글자색을 노란색으로 변경
			printf("Add 1 user. Now user number = %d\n", num_user);
			fprintf(stderr, "\033[32m");//글자색을 녹색으로 변경
			fprintf(stderr, "server>"); //커서 출력
		}

		// 클라이언트가 보낸 메시지를 모든 클라이언트에게 방송
		for (i = 0; i < num_user; i++) {
			if (FD_ISSET(clisock_list[i], &read_fds)) {
				num_chat++;				//총 대화 수 증가
				nbyte = recv(clisock_list[i], buf, MAXLINE, 0);
				if (nbyte <= 0) {
					removeClient(i);	// 클라이언트의 종료
					continue;
				}
				buf[nbyte] = 0;
				// 종료문자 처리
				if (strstr(buf, EXIT_STRING) != NULL) {
					removeClient(i);	// 클라이언트의 종료
					continue;
				}
				// 모든 채팅 참가자에게 메시지 방송

				for (j = 0; j < num_user; j++)
					send(clisock_list[j], buf, nbyte, 0);
				printf("\033[0G");		//커서의 X좌표를 0으로 이동
				fprintf(stderr, "\033[97m");//글자색을 흰색으로 변경
				printf("%s", buf);			//메시지 출력
				if(game_flag==1){
					if(strncmp(strchr(buf,'>'),city[nation_num],strlen(city[nation_num]))==0)
					{
						winner(buf);
						nation_num++;
						for (j = 0; j < num_user; j++){
							send(clisock_list[j], "Congraturation~\n", 200, 0);
						}
						if(nation_num < 5){
							strcpy(message,"");
							if(nation_name_flag[nation_num]==0){
								sleep(1);
								printf("%s Capital city? \n",nation[nation_num]);
								strcat(message,nation[nation_num]);
								strcat(message," Capital city?\n");
								nation_name_flag[nation_num]=1;
								for (j = 0; j < num_user; j++){
									printf("%d", j);
									send(clisock_list[j],message, strlen(message), 0);
								}
							strcpy(message,"");
							}
						}
						else{
							strcpy(message,"");
							sleep(1);
							findwinner();
							game_flag = 0;
							strcpy(message, "               The End of the Game Winner :  ");
							strcat(message, win);
							printf("%s", message);
							for(j=0; j<num_user; j++){
								//	send(clisock_list[j], "The End, Winner is  ", 200, 0);
									while(flag = send(clisock_list[j], message, strlen(message), 0) == -1){
										if(errno == EINTR)
											continue;
										else
											break;
									}
							}
						}
	
					}
				}
				fprintf(stderr, "\033[32m");//글자색을 녹색으로 변경
				fprintf(stderr, "server>"); //커서 출력
			}
		}

	}  // end of while

	return 0;
}

void findwinner(){

	int i,j, tmp=0, max=0;
	for(i=0; i<nation_num; i++){
		for(j=0; j<nation_num; j++)
			if(strncmp(user_list[i], user_list[j], strlen(user_list[i])) == 0)
				tmp++;
		if(tmp>max){
			max = tmp;
			strcpy(win,user_list[i]);
		}
		tmp = 0;
	}
}

void winner(char* buf){
	int i;
	buf = strchr(buf,']');
	for(i=1; i< strlen(buf); i++){
		if(buf[i] == '>'){
			strcat(user_list[nation_num], "");
			return;
		}
		strncat(user_list[nation_num],&buf[i],1);	
	}
	
}

void handle(int signum)
{
	signal(SIGALRM,handle);	
	printf("signum\n");

}

int set_ticker(int n_msecs)
{
	struct itimerval new_timeset;
	long n_sec, n_usecs;

	n_sec=n_msecs/1000;
	n_usecs = (n_msecs %1000) *1000;
 	new_timeset.it_interval.tv_sec=n_sec;
	new_timeset.it_interval.tv_usec=n_usecs;
	new_timeset.it_value.tv_sec=n_sec;
	new_timeset.it_value.tv_usec=n_usecs;
	return setitimer(ITIMER_REAL,&new_timeset,NULL);   
    
    
}
void *thread_function(void *arg) { //명령어를 처리할 스레드
	int i;
	char message[100];
	printf("Command : help, num_user, num_chat, ip_list game_start\n");
	while (1) {
		char bufmsg[MAXLINE + 1];
		fprintf(stderr, "\033[1;32m"); //글자색을 녹색으로 변경
		printf("server>"); //커서 출력
		fgets(bufmsg, MAXLINE, stdin); //명령어 입력
		if (!strcmp(bufmsg, "\n")) continue;   //엔터 무시
		else if (!strcmp(bufmsg, "help\n"))    //명령어 처리
			printf("help, num_user, num_chat, ip_list game_start\n");
		else if (!strcmp(bufmsg, "num_user\n"))//명령어 처리
			printf("User number = %d\n", num_user);
		else if (!strcmp(bufmsg, "num_chat\n"))//명령어 처리
			printf("The number of chatting = %d\n", num_chat);
		else if (!strcmp(bufmsg, "ip_list\n")) //명령어 처리
			for (i = 0; i < num_user; i++)
				printf("%s\n", ip_list[i]);
		else if(!strcmp(bufmsg, "game_start\n")) //game start 할 flag 변경
		{
			game_flag=1;
			printf("Game Start\n");
			game_start();
		}
		else //예외 처리
			printf("Please retry command, if you don't know command enter help.\n");
	}
}
void game_start()
{
	int i;
	char message[100];
	strcpy(message,"");
		if(nation_name_flag[nation_num]==0){
		printf("%s Capital City? \n",nation[nation_num]);
		strcat(message,nation[nation_num]);
		strcat(message," Capital City?\n");
		nation_name_flag[nation_num]=1;	
		for (i = 0; i < num_user; i++)
			send(clisock_list[i],message, 200, 0);
		strcpy(message,"");
	}
	
}
// 새로운 채팅 참가자 처리
void addClient(int s, struct sockaddr_in *newcliaddr) {
	char buf[20];
	inet_ntop(AF_INET, &newcliaddr->sin_addr, buf, sizeof(buf));
	write(1, "\033[0G", 4);		//커서의 X좌표를 0으로 이동
	fprintf(stderr, "\033[33m");	//글자색을 노란색으로 변경
	printf("new client: %s\n", buf);//ip출력
	// 채팅 클라이언트 목록에 추가
	clisock_list[num_user] = s;
	strcpy(ip_list[num_user], buf);
	num_user++; //유저 수 증가
}

// 채팅 탈퇴 처리
void removeClient(int s) {
	close(clisock_list[s]);
	if (s != num_user - 1) { //저장된 리스트 재배열
		clisock_list[s] = clisock_list[num_user - 1];
		strcpy(ip_list[s], ip_list[num_user - 1]);
	}
	num_user--; //유저 수 감소
	ct = time(NULL);			//현재 시간을 받아옴
	tm = *localtime(&ct);
	write(1, "\033[0G", 4);		//커서의 X좌표를 0으로 이동
	fprintf(stderr, "\033[33m");//글자색을 노란색으로 변경
	printf("[%02d:%02d:%02d]", tm.tm_hour, tm.tm_min, tm.tm_sec);
	printf("Out  1 User. Now User number = %d\n", num_user);
	fprintf(stderr, "\033[32m");//글자색을 녹색으로 변경
	fprintf(stderr, "server>"); //커서 출력
}

// 최대 소켓번호 찾기
int getmax() {
	// Minimum 소켓번호는 가정 먼저 생성된 listen_sock
	int max = listen_sock;
	int i;
	for (i = 0; i < num_user; i++)
		if (clisock_list[i] > max)
			max = clisock_list[i];
	return max;
}

// listen 소켓 생성 및 listen
int  tcp_listen(int host, int port, int backlog) {
	int sd;
	struct sockaddr_in servaddr;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket fail");
		exit(1);
	}
	// servaddr 구조체의 내용 세팅
	bzero((char *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(host);
	servaddr.sin_port = htons(port);
	if (bind(sd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("bind fail");  exit(1);
	}
	// 클라이언트로부터 연결요청을 기다림
	listen(sd, backlog);
	return sd;
} 
