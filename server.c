#include<stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <pthread.h>

#include "common.h"

sqlite3 *db;  //仅服务器使用,创建并打开数据库

int process_user_or_admin_login_request(int acceptfd,MSG *msg)
{
	printf("------------%s-----------%d.\n",__func__,__LINE__);
	//封装sql命令，表中查询用户名和密码－存在－登录成功－发送响应－失败－发送失败响应	
	char sql[DATALEN] = {0};
	char *errmsg;
	char **result;
	int nrow,ncolumn;

	msg->info.usertype =  msg->usertype;
	strcpy(msg->info.name,msg->username);
	strcpy(msg->info.passwd,msg->passwd);

	printf("usrtype: %#x-----usrname: %s---passwd: %s.\n",msg->info.usertype,msg->info.name,msg->info.passwd);
	sprintf(sql,"select * from usrinfo where usertype=%d and name='%s' and passwd='%s';",msg->info.usertype,msg->info.name,msg->info.passwd);
	if(sqlite3_get_table(db,sql,&result,&nrow,&ncolumn,&errmsg) != SQLITE_OK){
		printf("---****----%s.\n",errmsg);		
	}else{
		//printf("----nrow-----%d,ncolumn-----%d.\n",nrow,ncolumn);	
		if(nrow == 0){
			strcpy(msg->recvmsg,"name or passwd failed.\n");
			send(acceptfd,msg,sizeof(MSG),0);
		}else{
			strcpy(msg->recvmsg,"OK");
			send(acceptfd,msg,sizeof(MSG),0);
		}
	}
	return 0;	
}


int process_user_modify_request(int acceptfd,MSG *msg)
{
	char sql[DATALEN]={0};
	char *errmsg;
	switch(msg->flags){
	case 1 :
		sprintf(sql,"update usrinfo set passwd = '%s' where staffno = %d;",msg->info.passwd, msg->info.no);
		printf("sql:%s\n",sql);
		break;
	case 2:
		sprintf(sql,"update usrinfo set age = %d where staffno = %d;",msg->info.age, msg->info.no);
		printf("sql:%s\n",sql);
		break;
	case 3:
		sprintf(sql,"update usrinfo set phone = '%s' where staffno = %d;",msg->info.phone, msg->info.no);
		printf("sql:%s\n",sql);
		break;
	case 4:
		sprintf(sql,"update usrinfo set addr = '%s' where staffno = %d;",msg->info.addr, msg->info.no);
		printf("sql:%s\n",sql);
		break;
	}

	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
		printf("%s\n",errmsg);
		return -1;
	}
	else{
		printf("update success\n");
		strcpy(msg->recvmsg,"OK");
		msg->flags = 0;
	}
	if(send(acceptfd,msg,sizeof(MSG),0) <0){
		perror("server send failed\n");
		return -1;
	}
	return 0;
}

int process_user_query_request(int acceptfd,MSG *msg)
{


	char sql[DATALEN] = {0};
	char *errmsg;
	char **result;


	sprintf(sql,"select * from usrinfo where name = '%s' and passwd = '%s';"\
			,msg->info.name,msg->info.passwd);


	int callback(void * para,int f_num,char ** f_value, char **f_name){

		msg->info.no = atoi(f_value[0]);
		msg->info.usertype = atoi(f_value[1]);
		strcpy(msg->info.name,f_value[2]);
		strcpy(msg->info.passwd ,f_value[3]);
		msg->info.age = atoi(f_value[4]);
		strcpy(msg->info.phone , f_value[5]);
		strcpy(msg->info.addr , f_value[6]);
		strcpy(msg->info.work , f_value[7]);
		strcpy(msg->info.date ,f_value[8]);
		msg->info.level = atoi(f_value[9]);
		msg->info.salary = atoi(f_value[10]);
	}
	if((sqlite3_exec(db,sql,callback,"select",&errmsg))!= SQLITE_OK){		
		printf("%s --failed to select table ---- USER_QUERY",errmsg);
	}
	msg->flags = 1;

	if((send(acceptfd,msg,sizeof(MSG),0)) < 0){
		printf("server failed to send ---USER_QUERY\n");
	}
#if 0 
	if((sqlite3_get_table(db,sql,&result,&nrow,&ncolumn,&errmsg))!= SQLITE_OK){
		printf("%s --failed to select table ---- USER_QUERY",errmsg);
	}else{
		if(nrow == 0){
			strcpy(msg->recvmsg,"not find");
			send(acceptfd,msg,sizeof(MSG),0);
		}else{
			strcpy(msg->recvmsg,"OK");
			msg->flags = 1;

			for(i = 1 ;i < nrow + 1; i++){
				for(j = 0; j < ncolumn; j++ ){
					//msg->info.no=result[num++]
					printf("%s\t",result[num++]);
				}
				printf("\n");
			}	
			send(acceptfd,msg,sizeof(MSG),0);	
		}	
		return 0;
	}
#endif
}

int process_admin_modify_request(int acceptfd,MSG *msg)//管理员修改
{
	char *errmsg;
	char temp[10] = {0};
	char sql[DATALEN] = {0};
	printf("modify user: %s\n",msg->info.name);
	switch(msg->flags)
	{
	case 1:
		strcpy(temp,"staffno");
		break;
	case 2:
		strcpy(temp,"usertype");
		break;
	case 3:
		strcpy(temp,"name");
		break;
	case 4:
		strcpy(temp,"passwd");
		break;
	case 5:
		strcpy(temp,"age");
		break;
	case 6:
		strcpy(temp,"phone");
		break;
	case 7:
		strcpy(temp,"addr");
		break;
	case 8:
		strcpy(temp,"work");
		break;
	case 9:
		strcpy(temp,"date");
		break;
	case 10:
		strcpy(temp,"level");
		break;
	case 11:
		strcpy(temp,"salary");
		break;
	default:
		printf("input error");
	}

	sprintf(sql,"update usrinfo set '%s' = '%s' where name = '%s';",temp,msg->recvmsg,msg->info.name);

	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
		printf("update failed,%s\n",errmsg);
	strcpy(msg->recvmsg,"modify ok");
	send(acceptfd,msg,sizeof(MSG),0);

	return 0;

}

int process_admin_adduser_request(int acceptfd,MSG *msg)
{
	char * errmsg;
	char sql[DATALEN]={0};
	sprintf(sql,"insert into usrinfo values (%d,%d,'%s','%s',%d,'%s','%s','%s','%s',%d,%lf);",\
			msg->info.no, msg->info.usertype, msg->info.name, msg->info.passwd,\
			msg->info.age, msg->info.phone, msg->info.addr, msg->info.work,\
			msg->info.date, msg->info.level, msg->info.salary);

	printf("%s\n",sql);

	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
		printf("%s\n",errmsg);
		return -1;
	}
	else{
		printf("ADMIN_ADDUSER success\n");
		strcpy(msg->recvmsg,"ADMIN_ADDUSER");
		msg->flags = 1;
	}

	if(send(acceptfd,msg,sizeof(MSG),0) <0){
		perror("server send failed\n");
		return -1;
	}

}



int process_admin_deluser_request(int acceptfd,MSG *msg)
{
	char *errmsg;
	printf("-------%s---------%d\n",__func__,__LINE__);
	char sql[DATALEN] = {0};
	printf("delete user: %s\n",msg->info.name);

	sprintf(sql,"delete from usrinfo where name = '%s';",msg->info.name);

	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
		printf("delete failed,%s\n",errmsg);

	strcpy(msg->recvmsg,"delete ok");
	send(acceptfd,msg,sizeof(MSG),0);
	return 0;

}

int process_admin_query_request(int acceptfd,MSG *msg)
{
	char sql[DATALEN]={0};
	char *errmsg;
	char **result;
	int nrow,ncolumn;
	int i;
	sprintf(sql,"select * from usrinfo;");
	if(sqlite3_get_table(db,sql,&result,&nrow,&ncolumn,&errmsg)!=SQLITE_OK)	{
		printf("---****----%s.\n",errmsg);		
	}else{
		int num=ncolumn;
		for(i=0;i<nrow;i++){
			msg->info.no=atoi(result[num]);
			msg->info.usertype=atoi(result[num+1]);
			strcpy(msg->info.name,(char *)result[num+2]);
			strcpy(msg->info.passwd,(char *)result[num+3]);
			msg->info.age=atoi(result[num+4]);
			strcpy(msg->info.phone,(char *)result[num+5]);
			strcpy(msg->info.addr,(char *)result[num+6]);
			strcpy(msg->info.work,(char *)result[num+7]);
			strcpy(msg->info.date,(char *)result[num+8]);
			msg->info.level=atoi(result[num+9]);
			msg->info.salary=atoi(result[num+10]);
			send(acceptfd,msg,sizeof(MSG),0);
			num=num+11;
		}
		strcpy(msg->recvmsg,"ADMIN_QUERY");
		send(acceptfd,msg,sizeof(MSG),0);
		memset(msg->recvmsg,0,sizeof(msg->recvmsg));
	}
	return 0;
}

int process_admin_history_request(int acceptfd,MSG *msg)
{

	char sql[DATALEN]={0};
	char *errmsg;
	char **result;
	int nrow,ncolumn;
	int i;
	msg->flags = 0;
	sprintf(sql,"select * from historyinfo;");
	if(sqlite3_get_table(db,sql,&result,&nrow,&ncolumn,&errmsg)!=SQLITE_OK)	{
		printf("---****----%s.\n",errmsg);		
	}else{
		int num=ncolumn;
		for(i=0;i<nrow;i++){
			strcpy(msg->info.date,result[num]);
			strcpy(msg->username,(char *)result[num+1]);
			strcpy(msg->recvmsg,(char *)result[num+2]);
			send(acceptfd,msg,sizeof(MSG),0);
			num=num+3;
		}
		msg->flags = 1;
		send(acceptfd,msg,sizeof(MSG),0);
		memset(msg->recvmsg,0,sizeof(msg->recvmsg));
	}
	return 0;
#if 0	
	char *errmsg;
	char sql[DATALEN] = {0};
	char **result;

	sprintf(sql,"select * from historyinfo;");
	printf("sql:%s\n",sql);

	int callback(void * para,int f_num,char ** f_value, char **f_name){

		strcpy(msg->info.date, f_value[0]);
		strcpy(msg->username ,f_value[1]);
		strcpy(msg->recvmsg , f_value[2]);
		send(acceptfd,msg,sizeof(MSG),0);

	}

	if((sqlite3_exec(db,sql,callback,"QH",&errmsg))!= SQLITE_OK){
		msg->flags = 1;
		send(acceptfd,msg,sizeof(MSG),0);
		printf("%s --failed to select table ---- USER_QUERY",errmsg);
	}
	if((send(acceptfd,msg,sizeof(MSG),0)) < 0){
		printf("server failed to send ---ADMIN_HISTROY_QUERY\n");
	}
	return 0;
#endif
}

int process_client_quit_request(int acceptfd,MSG *msg)
{
	close(acceptfd);
	printf("Client QUIT.\n");
	return 0;

}


int process_client_request(int acceptfd,MSG *msg)
{
	switch (msg->msgtype)
	{
	case USER_LOGIN:
	case ADMIN_LOGIN:
		process_user_or_admin_login_request(acceptfd,msg);
		historyinfo_insert(msg,"LOGIN");
		break;
	case USER_MODIFY:
		process_user_modify_request(acceptfd,msg);
		historyinfo_insert(msg,"USER_MODIFY");
		break;
	case USER_QUERY:
		process_user_query_request(acceptfd,msg);
		historyinfo_insert(msg,"USER_QUERY");
		break;
	case ADMIN_MODIFY:
		process_admin_modify_request(acceptfd,msg);
		historyinfo_insert(msg,"ADMIN_MODIFY");
		break;

	case ADMIN_ADDUSER:
		process_admin_adduser_request(acceptfd,msg);
		historyinfo_insert(msg,"ADMIN_ADDUSER");
		break;

	case ADMIN_DELUSER:
		process_admin_deluser_request(acceptfd,msg);
		historyinfo_insert(msg,"ADMIN_DELUSER");
		break;
	case ADMIN_QUERY:
		process_admin_query_request(acceptfd,msg);
		historyinfo_insert(msg,"ADMIN_QUERY");
		break;
	case ADMIN_HISTORY:
		process_admin_history_request(acceptfd,msg);
		historyinfo_insert(msg,"ADMIN_HISTORY");
		break;
	case QUIT:
		process_client_quit_request(acceptfd,msg);
		break;
	default:
		break;
	}

}

void historyinfo_insert(MSG * msg,char * words){

	time_t now;
	struct tm *tm_now;
	char date[128]={'0'};
	time(&now);

	tm_now=localtime(&now);
	sprintf(date,"%d-%d-%d %d:%d:%d",tm_now->tm_year+1900,tm_now->tm_mon+1,tm_now->tm_mday,\
			tm_now->tm_hour,tm_now->tm_min,tm_now->tm_sec);
	char * errmsg;
	char sql[DATALEN]={0};
	sprintf(sql,"insert into historyinfo values ('%s','%s','%s');",\
			date, msg->username, words);
	if((sqlite3_exec(db,sql,NULL,NULL,&errmsg))!=SQLITE_OK){
		printf("history failed -- %s\n",errmsg);
	}
	return ;
}

int main(int argc, const char *argv[])
{
	//socket->填充->绑定->监听->等待连接->数据交互->关闭 
	int sockfd;
	int acceptfd;
	ssize_t recvbytes;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	socklen_t addrlen = sizeof(serveraddr);
	socklen_t cli_len = sizeof(clientaddr);

	MSG msg;
	//thread_data_t tid_data;
	char *errmsg;

	if(sqlite3_open(STAFF_DATABASE,&db) != SQLITE_OK){
		printf("%s.\n",sqlite3_errmsg(db));
	}else{
		printf("the database open success.\n");
	}

	if(sqlite3_exec(db,"create table usrinfo(staffno integer,usertype integer,name text,passwd text,age integer,phone text,addr text,work text,date text,level integer,salary REAL);",NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s.\n",errmsg);
	}else{
		printf("create usrinfo table success.\n");
	}

	if(sqlite3_exec(db,"create table historyinfo(time text,name text,words text);",NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s.\n",errmsg);
	}else{ 
		printf("create historyinfo table success.\n");
	}
	//创建网络通信的套接字
	sockfd = socket(AF_INET,SOCK_STREAM, 0);
	if(sockfd == -1){
		perror("socket failed.\n");
		exit(-1);
	}
	printf("sockfd :%d.\n",sockfd); 


	/*优化4： 允许绑定地址快速重用 */

	int b_reuse = 1;
	setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &b_reuse, sizeof (int));

	//填充网络结构体
	memset(&serveraddr,0,sizeof(serveraddr));
	memset(&clientaddr,0,sizeof(clientaddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port   = htons(5001);
	serveraddr.sin_addr.s_addr = inet_addr("192.168.10.118");


	//绑定网络套接字和网络结构体
	if(bind(sockfd, (const struct sockaddr *)&serveraddr,addrlen) == -1){
		printf("bind failed.\n");
		exit(-1);
	}

	//监听套接字，将主动套接字转化为被动套接字
	if(listen(sockfd,10) == -1){
				printf("listen failed.\n");
						exit(-1);
							}

	//定义一张表
	fd_set readfds,tempfds;
	//清空表
	FD_ZERO(&readfds);
	FD_ZERO(&tempfds);
	//添加要监听的事件
	FD_SET(sockfd,&readfds);
	int nfds = sockfd;
	int retval;
	int i = 0;
#if 0 //添加线程控制部分
	pthread_t thread[N];
	int tid = 0;
#endif
	while(1){
		tempfds = readfds;
		//记得重新添加
		retval =select(nfds + 1, &tempfds, NULL,NULL,NULL);
		//判断是否是集合里关注的事件
		for(i = 0;i < nfds + 1; i ++){
			if(FD_ISSET(i,&tempfds)){
				if(i ==sockfd){
					//数据交互 
					acceptfd = accept(sockfd,(struct sockaddr *)&clientaddr,&cli_len);
					if(acceptfd == -1){
						printf("acceptfd failed.\n");
						exit(-1);
					}
					printf("ip : %s.\n",inet_ntoa(clientaddr.sin_addr));
					FD_SET(acceptfd,&readfds);
					nfds = nfds > acceptfd ? nfds : acceptfd;
				}else{
					recvbytes = recv(i,&msg,sizeof(msg),0);
					printf("msg.type :%#x.\n",msg.msgtype);
					if(recvbytes == -1){
						printf("recv failed.\n");
						continue;
					}else if(recvbytes == 0){
						printf("peer shutdown.\n");
						close(i);
						FD_CLR(i, &readfds);  //删除集合中的i
					}else{
						process_client_request(i,&msg);
					}
				}
			}
		}
	}
	close(sockfd);

	return 0;
}



