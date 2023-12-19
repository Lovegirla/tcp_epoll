# include<stdio.h>
# include<unistd.h>
# include<stdlib.h>
# include<string.h>
# include<assert.h>
# include<sys/socket.h>
# include<netinet/in.h>
# include<arpa/inet.h>
int main()
{
	//创建套接子
	//AF_INFT表示ipv4  SOCKET_STREAM表示传输层使用tcp协议 
	int sockfd = socket(PF_INET,SOCK_STREAM,0);
	assert(sockfd!=-1);
 
	//绑定
	struct sockaddr_in saddr,caddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(6000);//1024知名  4096保留 5000之上，临时端口
	saddr.sin_addr.s_addr = inet_addr("0.0.0.0");
	int res = bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	assert(res!=-1);
 
	listen(sockfd,5);//开机
 
	while(1)
	{
		socklen_t len = sizeof(caddr);
		int c =accept(sockfd,(struct sockaddr*)&caddr,&len);
		if(c<0)
		{
			continue;
		}
		printf("c=%d,ip=%s\n",c,inet_ntoa(caddr.sin_addr));
 
		char buf[128]={0};
		int n = recv(c,buf,127,0);
		printf("buf(%d)=%s\n",n,buf);
		send(c,"ok",2,0);
 
		close(c);//待机
	}
 
	exit(0);
 
}