//#include <iostream>
#include <strings.h>
#include <string.h>
#include <sys/types.h>     
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
//using namespace std;
int get_local_ip(char* ip, char* localip){
	int fd, intrface, retn = 0;
	struct ifreq buf[INET_ADDRSTRLEN];
	struct ifconf ifc;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0){
		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = (caddr_t)buf;
		if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc)){
			intrface = ifc.ifc_len/sizeof(struct ifreq);
			while (intrface-- > 0){
				if (!(ioctl(fd, SIOCGIFADDR, (char *)&buf[intrface]))){
					ip=(inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));
					printf("IP:%s\n", ip);
					if(strstr(ip, "255") != NULL){
						printf("1 IP:%s\n", ip);
						continue;
					}
					else if(strstr(ip, ".0.0.") != NULL){
						printf("2 IP:%s\n", ip);
						continue;
					}
					else{
						printf("local IP:%s\n", ip);
						strcpy(localip, ip);
					}
				}
			}
		}
		close(fd);
		return 0;
	}
}
void encode(unsigned char* p, unsigned char* key){
  for(int i=0; i<strlen(p);i++){
  	//printf("%d %c \n" , p[i], p[i]);
	unsigned char ch = p[i];
	for(int j=0; j<strlen(key); j++){
		ch ^= key[j];
	}
	p[i] = ch;
  }
}
void decode(unsigned char* p, unsigned char* key){
  for(int i=0; i<strlen(p);i++){
  	//printf("%d %c \n" , p[i], p[i]);
	unsigned char ch = p[i];
	for(int j=0; j<strlen(key); j++){
		ch ^= key[j];
	}
	p[i] = ch;
  	//printf("%d %c \n" , p[i], p[i]);
  }
}
void get_ipkey(char* ip, char* ipb){
  
  char tmp[4];
  int pos = 0;
  int cnt = 0;
  for(int i=0; i<strlen(ip)+1;  i++)
  {
    	if(ip[i] == '.' || ip[i] == '\0'){
		tmp[pos]='\0';
		ipb[cnt++] = atoi(tmp);
		//printf("tmp is %s\n", tmp);
		pos = 0;
		memset(tmp, 0, sizeof(tmp));
	}
	else{
		tmp[pos++] = ip[i];
	}
  } 
}
int main(int argc, char **argv)
{
	int sockfd;
	int port;
	char ip[17];
	int interval;
	struct sockaddr_in des_addr;
	int r;
	unsigned char sendline[1024] = {"OVS"};
	const int on = 1;
        clock_t lasttime, nowtime;
	int tmpinterval;
	char testip[64];
	char localip[20];
	unsigned char ipb[4];
	memset(testip, 0, sizeof(testip));
	get_local_ip(testip, localip);
	get_ipkey(localip, ipb);
	printf("testip is %s\n", localip);
	if(argc != 4) return 0;
	else{
	    interval = atoi(argv[3]);
	    port = atoi(argv[2]);
	    strcpy(ip, argv[1]);
	    printf("port is %d ip is %s interval is %d\n", port, ip, interval);
	}
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)); //设置套接字选项
	bzero(&des_addr, sizeof(des_addr));
	des_addr.sin_family = AF_INET;
	des_addr.sin_addr.s_addr = inet_addr(ip); //广播地址
	des_addr.sin_port = htons(port);
	lasttime = clock();
	nowtime = lasttime;
	encode(sendline, ipb);
	r = sendto(sockfd, sendline, strlen(sendline), 0, (struct sockaddr*)&des_addr, sizeof(des_addr));
        while(1){
	    tmpinterval = (int)((double)(nowtime - lasttime)/CLOCKS_PER_SEC);
	    if(tmpinterval >= interval){ 
	        r = sendto(sockfd, sendline, strlen(sendline), 0, (struct sockaddr*)&des_addr, sizeof(des_addr));
	        if (r <= 0)
	        {
	           perror("");
		   exit(-1);
	        }
	        printf("send ok\n");
		lasttime = nowtime;
	    }
	    else{
	        nowtime = clock();
	    }
	}
	return 0;
}
