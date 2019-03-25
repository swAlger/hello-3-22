#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include <pthread.h>

#define N 512
#define DATABASE "database.db"

#define R  1
#define O  2
#define Q  3
#define M  4
#define A  5
#define D  6
#define H  7
#define T  8

#define NAME  9
#define ALL   10

//1. 定义msg结构体，把需要的各个成员变量协商好	
struct msg{
	int  id;
	char name[32];
	int  age;
	char family_addr[32];
	char telephone[32];
	char post[32];
	int  salary;
	int  hiredata;
	int  level;
	char passward[32];
	int  type;
	char text[N];
	int  flag;
	int  t_id;
	int  choice;
};

//1.1 定义全局变量，为添加历史纪录做准备
char name[32];
sqlite3 *db;

//1.2 获取系统时间函数，为添加历史纪录做准备
void get_system_time(char* timedata)
{
	time_t t;
	struct tm *tp;

	time(&t);
	tp = localtime(&t);
	sprintf(timedata,"%d-%d-%d %d:%d:%d",tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec);
}

//1.3 历史纪录信息添加函数，将用户的各种相应操作添加到历史记录表中
void process_history_handler(char *name, char *timedata, char *event, sqlite3 *db){
	printf("----------%s-------------_%d.\n",__func__,__LINE__);
	char sql[128] = {0};
	char *errmsg;
	
	sprintf(sql, "insert into historyinfo values('%s','%s', '%s');", name, timedata, event);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
		printf("%s.\n", errmsg);
	}else{
		printf("历史纪录更新成功\n");
	}
	
}

//4.1 用户登录函数，通过用户发送过来的消息，判断是普通还是管理员用户，并查找数据表，看信息是否正确
//		添加事件到历史记录表中
void process_login(int acceptfd, struct msg msg, sqlite3 *db, int flag){
	printf("----------%s-------------_%d.\n", __func__, __LINE__);
	char timedata[32] = {0};
	char event[128] = {0};
	char sql[128] = {0};
	char *errmsg;
	char **rep;
	int n_row;
	int n_column;

	sprintf(sql, "select * from usrinfo where name='%s' and passward='%s' and flag=%d;", msg.name, msg.passward, flag);
	if(sqlite3_get_table(db, sql, &rep, &n_row, &n_column, &errmsg) != SQLITE_OK){
		printf("%s.\n", errmsg);	
		return;
	}
	if(n_row == 0){
		strcpy(msg.text, "登陆名字或密码错误，请查实！\n");
		send(acceptfd, &msg, sizeof(msg), 0);
		printf("%s login failed.\n", msg.name);
		return;
	}else{
		strcpy(msg.text, "ok");
		send(acceptfd, &msg, sizeof(msg), 0);
		printf("%s login successed.\n", msg.name);
	}
	
	//添加事件到历史记录表
	strcpy(name, msg.name);
	get_system_time(timedata);
	if(flag == 1){
		strcpy(event, "管理员登陆成功");
	}else{
		strcpy(event, "用户登录成功");
	}
	process_history_handler(name, timedata, event, db);
}

//4.2 查询用户信息，通过用户发送过来的消息，判断是按名字查找还是查找所有。
//		将查找到的内容发送给客户端
//		添加事件到历史记录表中
void process_query(int acceptfd, struct msg msg, sqlite3 *db){
	printf("----------%s-------------_%d.\n", __func__, __LINE__);
	char timedata[32] = {0};
	char event[128] = {0};
	char sql[128] = {0};
	char *errmsg;
	char **rep;
	int n_row;
	int n_column;
	int i,j;
	
	strcpy(msg.text, "进入查询功能成功.");
	send(acceptfd, &msg, sizeof(msg),0);
	
	while(1){
		memset(msg.text, 0, sizeof(msg.text));
		msg.flag = 0;
		recv(acceptfd, &msg, sizeof(msg), 0);
		printf("msg.type :%d.\n", msg.type);
		
		if(msg.type == NAME){
		//通过名字查询
		SELECT_NAME:
            printf("%s, %d\n", msg.name, msg.flag);
			sprintf(sql, "select * from usrinfo where name='%s' and flag=%d;", msg.text, msg.flag);
			if(sqlite3_get_table(db, sql, &rep, &n_row, &n_column, &errmsg) != SQLITE_OK){
				printf("%s\n", errmsg);
				goto SELECT_NAME;
			}else{
				if(n_row == 0){
					printf("query fail.\n");
					strcpy(msg.text,"没查找到，请确认输入信息是否正确!\n");
				}else{
					//添加事件到历史记录表
					get_system_time(timedata);
					strcpy(event, "该用户查询了");
					strcat(event, msg.text);
					process_history_handler(name, timedata, event, db);
					
					//遍历员工信息数据库，将需要查找的用户信息发送给客户
					memset(msg.text, 0, sizeof(msg.text));
					for(i=0; i<n_row+1; i++){
						for(j=0; j<n_column; j++){
							strcat(msg.text, *rep);
							strcat(msg.text, "  ");
							rep++;
						}
						strcat(msg.text, "\n");
					}
					printf("query success.\n");
					strcat(msg.text, "\n查找成功\n");
				}
				send(acceptfd, &msg,sizeof(msg), 0);
			}
		}else if(msg.type == ALL){
		//查询所有信息
		SELECT_ALL:
			sprintf(sql, "select * from usrinfo;");
			if(sqlite3_get_table(db, sql, &rep, &n_row, &n_column, &errmsg) != SQLITE_OK){
				printf("%s\n", errmsg);
				goto SELECT_ALL;
			}else{
				if(n_row == 0){
					printf("query fail.\n");
					strcpy(msg.text, "数据库已空\n");
				}else{
					//添加事件到历史记录表
					get_system_time(timedata);
					strcpy(event, "管理员查询了");
					strcat(event, msg.text);
					process_history_handler(name, timedata, event, db);
					
					//遍历员工信息数据库，将所有信息发送给客户
					for(i=0; i<n_row+1; i++){
						for(j=0; j<n_column; j++){
							strcat(msg.text, *rep);
							strcat(msg.text, "  ");
							rep++;
						}
						strcat(msg.text, "\n");
					}
					printf("query success.\n");
					strcat(msg.text, "\n查找成功\n");
				}
				send(acceptfd,&msg, sizeof(msg), 0);
			}
		}else if(msg.type == T){
		//用户返回上一级界面
			break;
		}
	}
}

//4.3 修改用户信息，通过用户发送过来的消息，判断需要修改的是哪一项内容，
//		并对发送过来的消息进行处理，然后进行相应的修改。
//		添加事件到历史记录表中
void process_modify(int acceptfd, struct msg msg, sqlite3 *db){
	printf("----------%s-------------_%d.\n", __func__, __LINE__);
	char timedata[32] = {0};
	char event[128] = {0};
	char sql[128] = {0};
	char *errmsg;

	if(msg.choice == 1){
		sprintf(sql, "update usrinfo set name='%s' where id=%d;", msg.text, msg.t_id);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
			goto ERR;
		}
	}else if(msg.choice == 2){
		sprintf(sql, "update usrinfo set age=%d where id=%d;", atoi(msg.text), msg.t_id);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
			goto ERR;
		}
	}else if(msg.choice == 3){
		sprintf(sql, "update usrinfo set family_addr='%s' where id=%d;", msg.text, msg.t_id);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
			goto ERR;
		}
	}else if(msg.choice == 4){
		sprintf(sql, "update usrinfo set telephone='%s' where id=%d;", msg.text, msg.t_id);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
			goto ERR;
		}
	}else if(msg.choice == 5){
		sprintf(sql, "update usrinfo set post='%s' where id=%d;", msg.text, msg.t_id);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
			goto ERR;
		}
	}else if(msg.choice == 6){
		sprintf(sql, "update usrinfo set salary=%d where id=%d;", atoi(msg.text), msg.t_id);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
			goto ERR;
		}
	}else if(msg.choice == 7){
		sprintf(sql, "update usrinfo set hiredata=%d where id=%d;", atoi(msg.text), msg.t_id);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
			goto ERR;
		}
	}else if(msg.choice == 8){
		sprintf(sql, "update usrinfo set level=%d where id=%d;", atoi(msg.text), msg.t_id);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
			goto ERR;
		}
	}else if(msg.choice == 9){
		sprintf(sql, "update usrinfo set passward='%s' where id=%d;", msg.text, msg.t_id);
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
			goto ERR;
		}
	}
	
	strcpy(msg.text,"数据修改成功");
	send(acceptfd,&msg,sizeof(msg),0);
	printf("id:%d, data update success.\n", msg.t_id);
	
	//添加事件到历史记录表
	get_system_time(timedata);
	strcpy(event, "修改了信息");
	process_history_handler(name, timedata, event, db);
	return;
	
ERR:
	strcpy(msg.text, "数据修改失败，请确认输入的工号或其他信息是否正确！");
	send(acceptfd, &msg, sizeof(msg), 0);
	printf("%s.\n", errmsg);
}

//4.4 添加用户信息，通过用户发送过来的消息，添加一个新用户，并反馈信息给客户端
//		添加事件到历史记录表中
void process_add(int acceptfd, struct msg msg, sqlite3 *db){
	printf("----------%s-------------_%d.\n",__func__,__LINE__);
	char timedata[32] = {0};
	char event[128] = {0};
	char sql[128] = {0};
	char *errmsg;

	sprintf(sql, "insert into usrinfo values(%d,'%s', %d, '%s', '%s', '%s', %d, %d, %d, '%s', %d);", msg.t_id, msg.name, msg.age, msg.family_addr, msg.telephone, msg.post, msg.salary, msg.hiredata, msg.level, msg.passward, msg.flag);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
		printf("%s.\n", errmsg);
		strcpy(msg.text, "用户添加失败");
		send(acceptfd,&msg,sizeof(msg),0);
		return;
	}else{
		strcpy(msg.text,"用户添加成功");
		send(acceptfd,&msg,sizeof(msg),0);
		printf("%s insert success.\n",msg.name);
	}
	
	//添加事件到历史记录表
	get_system_time(timedata);
	strcpy(event, "管理员添加了新用户");
	strcat(event, msg.name);
	process_history_handler(name, timedata, event, db);
}

//4.5 删除用户信息，通过用户发送过来的消息，删除一个用户，并反馈信息给客户端
//		添加事件到历史记录表中
void process_del(int acceptfd, struct msg msg, sqlite3 *db){
	printf("----------%s-------------_%d.\n",__func__,__LINE__);
	char timedata[32] = {0};
	char event[128] = {0};
	char sql[128] = {0};
	char *errmsg;

	sprintf(sql, "delete from usrinfo where id=%d", msg.t_id);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
		printf("%s.\n", errmsg);
		strcpy(msg.text, "删除用户失败");
		send(acceptfd,&msg,sizeof(msg),0);
		return;
	}else{
		strcpy(msg.text,"删除用户成功");
		send(acceptfd,&msg,sizeof(msg),0);
		printf("delete success.\n");
	}
	
	//添加事件到历史记录表
	get_system_time(timedata);
	strcpy(event, "管理员删除了一个用户");
	process_history_handler(name, timedata, event, db);
}

//4.6 历史记录查询，通过用户发送过来的消息，将查找到的历史纪录循环发送给客户端
//		添加事件到历史记录表中
void process_history(int acceptfd, struct msg msg, sqlite3 *db){
	printf("----------%s-------------_%d.\n",__func__,__LINE__);
	char timedata[32] = {0};
	char event[128] = {0};
	char sql[128] = {0};
	char *errmsg;
	char **rep;
	int n_row;
	int n_column;
	int i,j;

	sprintf(sql,"select * from historyinfo;");
	if(sqlite3_get_table(db, sql, &rep, &n_row, &n_column, &errmsg) != SQLITE_OK){
		printf("%s\n", errmsg);
		return;
	}else{
		if(n_row == 0){
			printf("query fail.\n");
			strcpy(msg.text, "end");
		}else{
			for(i=0; i<n_row+1; i++){
				for(j=0; j<n_column; j++){
					strcat(msg.text, *rep);
					if(j < (n_column-1)){
						strcat(msg.text, ",");
					}
					rep++;
				}
				send(acceptfd,&msg, sizeof(msg), 0);
				memset(msg.text, 0, sizeof(msg.text));
			}
			printf("query success.\n");
			strcpy(msg.text, "历史纪录查询成功\n");
			send(acceptfd,&msg, sizeof(msg), 0);
			strcpy(msg.text, "end");
		}
		send(acceptfd,&msg, sizeof(msg), 0);
		
		//添加事件到历史记录表
		get_system_time(timedata);
		strcpy(event, "管理员查询了历史纪录");
		process_history_handler(name, timedata, event, db);
		return;
	}
}

//4.7 用户选择退出程序，则关闭acceptfd描述符
void process_quit(int acceptfd, struct msg msg, sqlite3 *db){
	printf("----------%s-------------_%d.\n", __func__, __LINE__);
	strcpy(msg.text, "quit");
	send(acceptfd, &msg, sizeof(msg), 0);
	close(acceptfd);
}

//3. 线程处理函数，在这里处理各个用户的交互信息。
//	 通过服务器发送过来的功能选项跳转到对应的函数里
void *  client_request_handler(void * args){
	int acceptfd = *(int *)args;
	struct msg msg;

	while(1){
		//数据交互 
		//接收用户的选项，用户名及密码
		msg.type = 0;
		recv(acceptfd, &msg, sizeof(msg), 0);
		printf("msg.type :%d.\n", msg.type);
		switch(msg.type){
			case R:
				process_login(acceptfd, msg, db, 1);
				break;
			case O:
				process_login(acceptfd, msg, db, 0);
				break;
			case Q:
				process_query(acceptfd, msg, db);
				break;
			case M:
				process_modify(acceptfd, msg, db);
				break;
			case A:
				process_add(acceptfd, msg, db);
				break;
			case D:
				process_del(acceptfd, msg, db);
				break;
			case H:
				process_history(acceptfd, msg, db);
				break;
			case T:
				process_quit(acceptfd, msg, db);
				break;
		}
		if(msg.type == T || msg.type == 0){
			break;
		}
	}
	return (void *)(-1);
}

// 2. 在主函数里首先通过TCP连接流程，
//    分别实现socket->填充->绑定->监听->等待连接->数据交互->关闭
//	  创建一个数据库，添加两张表
//	  添加一个root用户，为之后的功能做准备
//	  通过select监听多个描述符，并创建多个线程，在线程调用函数里完成各个客户端的功能，实现多线程通信
int main(int argc, const char *argv[])
{
//	2.1 定义相应的变量
	int sockfd;
	int acceptfd;
	ssize_t recvbytes;
	char buf[N] = {0};
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	socklen_t ser_len = sizeof(serveraddr);
	socklen_t cli_len = sizeof(clientaddr);
	
	//2.2 初始化一个管理员模式的用户root
	struct msg msg = {
		.id = 0,
		.name = "root",
		.age = 32,
		.family_addr = "xxx_yuexiu_Street",
		.telephone = "132xxxxxxxx",
		.post = "root",
		.salary = 50000,
		.hiredata = 20180505,
		.level = 3,
		.passward = "root",
		.flag = 1,
	};
	
	/* 2.3
	 * TCP通信流程
	 * socket->填充->绑定->监听->等待连接->数据交互->关闭 
	 */
	sockfd = socket(AF_INET,SOCK_STREAM, 0);
	if(sockfd == -1){
		perror("socket failed.");
		exit(-1);
	}
	printf("sockfd :%d.\n",sockfd); 

	memset(&serveraddr, 0, ser_len);
	memset(&clientaddr, 0, cli_len);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port   = htons(atoi(argv[2]));
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);

	if(bind(sockfd, (const struct sockaddr *)&serveraddr, ser_len) == -1){
		perror("bind failed.");
		exit(-1);
	}

	if(listen(sockfd, 15) == -1){
		perror("listen failed.");
		exit(-1);
	}

	/*2.4
	 *创建一个数据库，添加两张表，一个用户存放员工信息，一个用于存放历史纪录
	 */
	char sql[N] = {0};
	char *errmsg;

	if(sqlite3_open(DATABASE,&db) != SQLITE_OK){
		printf("%s.\n", sqlite3_errmsg(db));
	}else{
		printf("the database open success.\n");
	}

	if(sqlite3_exec(db, "create table usrinfo(id int primary key, name text, age int, family_addr text, telephone text, post text, salary int , hiredata int, level int, passward text, flag int);",NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s.\n", errmsg);
	}else{
		printf("create usrinfo table success.\n");
	}

	if(sqlite3_exec(db, "create table historyinfo(name text,time text,event text);",NULL,NULL,&errmsg)!= SQLITE_OK){
		printf("%s.\n", errmsg);
	}else{
		printf("create historyinfo table success.\n");
	}

	//2.5 自行添加一个人root用户
	sprintf(sql,"insert into usrinfo values(%d,'%s', %d, '%s', '%s', '%s', %d, %d, %d, '%s', %d);",msg.id, msg.name, msg.age, msg.family_addr, msg.telephone, msg.post, msg.salary, msg.hiredata, msg.level, msg.passward, msg.flag);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
		printf("%s.\n", errmsg);
	}else{
		printf("root online success.\n");
	}
	
	
	//2.6 通过select实现多线程通信
	
	//定义一张表
	fd_set readfds;
	//清空表
	FD_ZERO(&readfds);
	//添加要监听的事件
	FD_SET(0,&readfds);
	FD_SET(sockfd,&readfds);
	int nfds = sockfd;
	int retval;
	int i = 0;

	//select和多线程的结合使用
	pthread_t thread[N];
	int tid = 0;

	while(1){
		//记得重新添加
		FD_SET(0,&readfds);
		FD_SET(sockfd,&readfds);
		retval =select(nfds + 1, &readfds, NULL,NULL,NULL);
		//判断是否是集合里关注的事件
		for(i = 0;i <= nfds; i ++){
			if(FD_ISSET(i,&readfds)){
				if(i == 0){
					fgets(buf,sizeof(buf),stdin);
					buf[strlen(buf) - 1] = '\0';
					printf("******%s---gaga.\n",buf);
				}
				if(i == sockfd){
					//数据交互 
					acceptfd = accept(sockfd,(struct sockaddr *)&clientaddr,&cli_len);
					if(acceptfd == -1){
						printf("acceptfd failed.\n");
						exit(-1);
					}
					printf("ip : %s.\n",inet_ntoa(clientaddr.sin_addr));
					pthread_create(&thread[tid++],NULL,client_request_handler,(void *)&acceptfd);
				}
			}
		}
	}
	
	close(sockfd);

	return 0;
}
