//#include <iostream>
#include <strings.h>
#include <string.h>
#include <sys/types.h>     
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
//using namespace std;
#define SQL_SELECT 0
#define SQL_INSERT 1
#define SQL_DELETE 2
#define SQL_CHECK  3

char removeIP[20][20];
int pos = 0;

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
void resetRemoveIP(){
	for(int i = 0; i < 20; i++)
	{
		memset(removeIP[i], 0, sizeof(removeIP[i]));
	}
}
static void setnonblocking(int sockfd) {
	int flag = fcntl(sockfd, F_GETFL, 0);
	if (flag < 0) {
		printf("get sockfd failed\n");
		return;
	}
	if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0) {
		printf("fcntl F_SETFL failed\n");
	}
}
int checkOVSIP(char* ip){
	//struct sockaddr_in addr;
	//int fd = socket(AF_INET, SOCK_STREAM, 0);
        
	for(int i = 0; i < pos; i++){
		if(strcmp(ip, removeIP[i]) == 0){
			printf("ip %s is dup\n, ignore\n");
			return -1;
		}
	}
	//setnonblocking(fd);
	struct sockaddr_in addr;
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct timeval tv_out;
	tv_out.tv_sec = 0;
	tv_out.tv_usec = 1;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6640);
	addr.sin_addr.s_addr = inet_addr(ip);

	if(connect(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr)) < 0){
	        printf("ip %s connect failed\n", ip);
		return 0;
	}

	return 1;
}

static int callback(void *data, int argc, char **argv, char **azColName){
	int i;
	// pos = 0;
	// resetRemoveIP();
	for(i = 0; i < argc; i++){
		//TBD
		printf("i=%d %s = %s \n", i, azColName[i], argv[i] ? argv[i] : "NULL", argc);
		if(checkOVSIP(argv[i]) ==  0){

			strcpy(removeIP[pos++], argv[i]);			
		}

	}
	for(i = 0; i < pos; i++)
	{
		printf("index %d, IP is %s pos %d\n", i, removeIP[i], pos);
	}
	return 0;

}
char* get_sqlcmd(int type, char* ip){
	char *cmd = NULL;
	cmd = (char*) malloc(200*sizeof(char));
	memset(cmd, 0, sizeof(cmd));
	if(cmd == NULL)
	{
	 	printf("[%s:%d]malloc resource failed\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	if(type == SQL_SELECT){
		strcat(cmd, "SELECT * from openvswitch_openvswitch where ip_address = '");
		strcat(cmd, ip);
		strcat(cmd, "'");	
	}
	else if(type == SQL_INSERT){
		strcat(cmd, "INSERT INTO openvswitch_openvswitch (protocol, ip_address, port, socket_file, database, is_default, owner_id)");
		strcat(cmd, "VALUES ('tcp', '");
		strcat(cmd, ip);
		strcat(cmd, "', 6640, 'NULL', 'Open_vSwitch', 0, 1)");
	}
	else if(type == SQL_DELETE){
		strcat(cmd, "DELETE from openvswitch_openvswitch where ip_address = '");
		strcat(cmd, ip);
		strcat(cmd, "'");	
	}
	else if(type == SQL_CHECK){
		strcat(cmd, "SELECT ip_address from openvswitch_openvswitch");
	}
	return cmd;
}

int free_sqlcmd(char* cmd){
	if(cmd == NULL){
		printf("cmd is NULL, no need to free\n");
	}
	else{
		free(cmd);
	}
}

static void setnonblocking0(int sockfd) {
	int flag = fcntl(sockfd, F_GETFL, 0);
	if (flag < 0) {
		printf("get sockfd failed\n");
		return;
	}
	if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0) {
		printf("fcntl F_SETFL failed\n");
	}
}

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in saddr;
	int r;
	unsigned char recvline[1025];
	unsigned char ipb[4];
	struct sockaddr_in presaddr;
	socklen_t len;
	int port;
	char dbname[120];
	sqlite3 *db;
        //strcpy(dbname, "/root/projects/vtap/db.sqlite3");
	const char* data = "Callback function called";
	char *zErrMsg = 0;
        clock_t lasttime, nowtime;
	int tmpinterval;
	if(argc == 2){
	    port = atoi(argv[1]);
            strcpy(dbname, "/root/projects/vtap/db.sqlite3");
	    printf("udp server listen port is %d\n", port);
	}
	else if(argc ==3){
	    port = atoi(argv[1]);
            strcpy(dbname, argv[2]);
	    printf("udp server listen port is %d\n", port);
	}
	else{
	    return 0;
	}
	
	printf("db is %s\n" , dbname);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(port);

	setnonblocking(sockfd);
	bind(sockfd, (struct sockaddr*)&saddr, sizeof(saddr));

	lasttime = clock();
	nowtime = lasttime;
	while (1)
	{
		tmpinterval = (int)((double)(nowtime - lasttime)/CLOCKS_PER_SEC);
		if( tmpinterval >= 10){
		    printf("tminterval greater 10s, check database if need to remove\n");	
		    //int rc = sqlite3_open("db.sqlite3", &db);
		    int rc = sqlite3_open(dbname, &db);
		    if(rc != SQLITE_OK){
		        printf("[%s:%d]open failed\n", __FUNCTION__, __LINE__);
		    	sqlite3_close(db);
		    }

		    pos = 0;
		    resetRemoveIP();
		    char* cmd = get_sqlcmd(SQL_CHECK, inet_ntoa(presaddr.sin_addr));
		    rc = sqlite3_exec(db, cmd, callback, (void*)data, &zErrMsg);
		    sqlite3_close(db);
		    free_sqlcmd(cmd);
		    
		    for(int i = 0; i < pos; i++){
		    	printf("del ip is %s\n", removeIP[i]);
		    }
		    //rc = sqlite3_open("db.sqlite3", &db);
		    rc = sqlite3_open(dbname, &db);
		    if(rc != SQLITE_OK){
		        printf("[%s:%d]open failed\n", __FUNCTION__, __LINE__);
		    	sqlite3_close(db);
		    }
		    for(int i=0; i < pos; i++){
		    		
		    	cmd = get_sqlcmd(SQL_DELETE, removeIP[i]);
		    	rc = sqlite3_exec(db, cmd, NULL, NULL, NULL);
			printf("del ip %s rc %d SQLITE_OK %d\n", removeIP[i], rc, SQLITE_OK);
			free_sqlcmd(cmd);
		    }
		    sqlite3_close(db);
		    lasttime = nowtime;
		}	
		else{
		    nowtime = clock();
		}

		r = recvfrom(sockfd, recvline, sizeof(recvline), 0 , (struct sockaddr*)&presaddr, &len);
		if (r <= 0)
		{
			continue;
		}
		recvline[r] = 0;
		//printf("%s len %d\n", recvline, strlen(recvline));
		get_ipkey(inet_ntoa(presaddr.sin_addr), ipb);
		decode(recvline, ipb);
		if(strcmp(recvline, "OVS") == 0){
		    if(strstr(inet_ntoa(presaddr.sin_addr), "255") != NULL){
		    	printf("recvfrom gateway %s %s \n", inet_ntoa(presaddr.sin_addr), recvline);
			continue;
		    }
		    //int rc = sqlite3_open("db.sqlite3", &db);
		    int rc = sqlite3_open(dbname, &db);
		    if(rc != SQLITE_OK){
			printf("[%s %d]open failed\n", __FUNCTION__, __LINE__);
		    	sqlite3_close(db);
		    }
		    else{
			char* cmd = get_sqlcmd(SQL_SELECT, inet_ntoa(presaddr.sin_addr));
		        //rc = sqlite3_exec(db, cmd, callback, (void*)data, &zErrMsg);
		        rc = sqlite3_exec(db, cmd, NULL, NULL, NULL);
		        if(rc != SQLITE_OK){
			    printf("database has no  %s, Plz insert it\n", inet_ntoa(presaddr.sin_addr));
		    	    sqlite3_close(db);
			    free_sqlcmd(cmd);
		    	    //rc = sqlite3_open("db.sqlite3", &db);
		    	    rc = sqlite3_open(dbname, &db);
		    	    if(rc != SQLITE_OK){
				   printf("[%s:%d]open failed\n", __FUNCTION__, __LINE__);
		    		   sqlite3_close(db);
		    	    }
			    char* cmdinsert = get_sqlcmd(SQL_INSERT, inet_ntoa(presaddr.sin_addr));
		            //rc = sqlite3_exec(db, cmdinsert, callback, (void*)data, &zErrMsg);
		            rc = sqlite3_exec(db, cmdinsert, NULL, NULL, NULL);
			    if(rc != SQLITE_OK){
			    	printf("%s insert failed\n", cmdinsert);
			    }
			    sqlite3_close(db);
			    free_sqlcmd(cmdinsert);
		        }
                        else{
			    printf("database has %s, Skip\n", inet_ntoa(presaddr.sin_addr));
			    sqlite3_close(db);
		    	    free_sqlcmd(cmd);
			}
		    }
		}
		else {
		
		    printf("recvfrom2 %s %s \n", inet_ntoa(presaddr.sin_addr), recvline);
		}
	}
	return 0;
}
