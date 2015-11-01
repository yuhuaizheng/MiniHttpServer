#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

char *blogHtml;
int bufLen;

void *HttpResponse(void *client)
{
	int client_fd = *(int*)client;
	char recvBuf[1024];
	recv(client_fd, recvBuf, sizeof(recvBuf), 0);
	send(client_fd, blogHtml, bufLen, 0);
	shutdown(client_fd, SHUT_RDWR);
	close(client_fd);
	pthread_exit((void *)0);
}

void *ListenClient(void *server)
{
	int server_fd = *(int*)server;
	int len = sizeof(struct sockaddr);
	struct sockaddr_in client_addr;
	pthread_t ntid;
	while(1)
	{
		int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
		struct timeval timeout = {5, 0};
		setsockopt(client_fd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
		setsockopt(client_fd,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout,sizeof(struct timeval));
		char buf[30];
		printf("connect:%s:%d\n",inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof(buf)),(unsigned int)ntohs(client_addr.sin_port));
		pthread_create(&ntid, NULL, HttpResponse, &client_fd);
		pthread_detach(ntid);
	}	
}


void HtmlInit()
{
	FILE *fp;
	fp = fopen("www/blogs.html", "r");
	fseek(fp, 0, SEEK_END);
	int len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	blogHtml = (char *)malloc((len + 100) * sizeof(char));
	char sendBuf[] = "HTTP/1.1 200 OK\r\nServer:BlogServer\r\nContent-Length:";
	strcpy(blogHtml, sendBuf);
	sprintf(blogHtml + sizeof(sendBuf) - 1, "%d", len);
	strcat(blogHtml, "\r\n\r\n");
	fread(blogHtml + strlen(blogHtml), len, sizeof(char), fp);
	fclose(fp);
	bufLen = strlen(blogHtml);
}

int main(int argc,char* argv[])
{
	if(argc != 2) return 1;
	int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	struct sockaddr_in server_addr;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[1]));
	int val = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(int));
	if(!bind(server_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr))) {char buf[30];printf("bind:%s:%d\n",inet_ntop(AF_INET, &server_addr.sin_addr, buf, sizeof(buf)),(unsigned int)ntohs(server_addr.sin_port));}
	else return 1;
	if(!listen(server_fd, 100)) {char buf[30];printf("listen:%s:%d\n",inet_ntop(AF_INET, &server_addr.sin_addr, buf, sizeof(buf)),(unsigned int)ntohs(server_addr.sin_port));}
	else return 1;

	HtmlInit();
	printf("load finish!\n");

	pthread_t ntid;
	int ret = pthread_create(&ntid, NULL, ListenClient, &server_fd);
	if(ret){ printf("thread create error!\nerror code = %d", ret); return -1; }

	while(1) sleep((unsigned)1000);

	free(blogHtml);

	close(server_fd);
	return 0;
}


