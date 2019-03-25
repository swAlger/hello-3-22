#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define N 512

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

//3.1 查询信息，制作界面，并通过提示信息让用户输入相应的功能选项。
//		判断并将相应的选项发送给服务器，
//		发送两次，第一次发送查询选项，第二次发送子功能选项，然后等待接收服务器的反馈消息
//		选择退出，则跳出循环，返回上一级界面
void do_query(int sockfd,struct msg msg){
	char flags = 0;
	msg.type = Q;
	send(sockfd, &msg, sizeof(msg), 0);
	recv(sockfd, &msg, sizeof(msg), 0);
	printf("%s\n", msg.text);
	
	while(1){
		int choice = 0;

		printf("***************************************\n");
		printf("**1 按人名查找  2 查找所有  3 退出*****\n");
		printf("***************************************\n");

		printf("请输入你的选择>>>");
		scanf("%d", &choice);
		getchar();
		memset(msg.text, 0, sizeof(msg.text));

		if(choice == 1){
			msg.type = NAME;
			printf("请输入您要查找的用户名:");
			scanf("%s", msg.text);
			getchar();
			printf("这个人是否是管理员(Y/N):");
			scanf("%c", &flags);
			getchar();

			if(flags == 'Y' || flags == 'y'){
				msg.flag = 1;
            }else{
				msg.flag = 0;
             }

			send(sockfd, &msg, sizeof(msg), 0);
			recv(sockfd, &msg, sizeof(msg), 0);
			printf("%s\n", msg.text);
		}else if(choice == 2){
			msg.type = ALL;
			strcpy(msg.text, "所有用户");
			send(sockfd, &msg, sizeof(msg), 0);
			recv(sockfd, &msg, sizeof(msg), 0);
			printf("%s\n", msg.text);
		}else if(choice == 3){
			msg.type = T;
			send(sockfd, &msg, sizeof(msg), 0);
			break;
		}
	}	
}

//3.2 修改信息，通过界面信息提示用户输入选项，
//		将相应的内容发送给服务器，等待接收服务器的反馈消息
void do_modify(int sockfd, struct msg msg){
	printf("请输入您要修改的指定的工号 >>>");
	scanf("%d", &msg.t_id);
	getchar();
	
	while(1){
		msg.type = M;
		printf("************************************************\n");
		printf("***1 名字   2 年龄   3 家庭住址   4 电话号码***\n");
		printf("***5 职位   6 薪水   7 入职日期   8 评级*******\n");
		printf("***9 密码                        10 退出*******\n");
		printf("**************************************************\n");
		printf("请输入您的选择 >>>");
		scanf("%d", &msg.choice);
		getchar();
		
		if(msg.choice == 10){
			break;
		}
		
		printf("请输入新的内容 >>>");
		scanf("%s", msg.text);
		getchar();
		send(sockfd, &msg, sizeof(msg), 0);
		
		recv(sockfd, &msg, sizeof(msg), 0);
		printf("%s\n", msg.text);
	}
}

//3.3 添加用户信息，通过界面信息提示用户输入选项，
//		将相应的内容发送给服务器
void do_add(int sockfd, struct msg msg){
	char choice;
	msg.type = A;
	printf("请输入工号:");
	scanf("%d", &msg.t_id);
	getchar();
	printf("您输入的工号是：%d\n工号信息一旦录入无法更改，请确认您所输入的是否正确！(Y/N):", msg.t_id);
	scanf("%c", &choice);
	getchar();
	if(choice == 'Y' || choice == 'y'){
		printf("请输入名字:");
		scanf("%s", msg.name);
		getchar();
		printf("请输入年龄:");
		scanf("%d", &msg.age);
		getchar();
		printf("请输入家庭住址:");
		scanf("%s", msg.family_addr);
		getchar();
		printf("请输入电话号码:");
		scanf("%s", msg.telephone);
		getchar();
		printf("请输入职位:");
		scanf("%s", msg.post);
		getchar();
		printf("请输入薪水:");
		scanf("%d", &msg.salary);
		getchar();
		printf("请输入入职日期:");
		scanf("%d", &msg.hiredata);
		getchar();
		printf("请输入评级:");
		scanf("%d", &msg.level);
		getchar();
		printf("请输入密码:");
		scanf("%s", msg.passward);
		getchar();
		printf("这个人是否是管理员(1(是)/0(不是)):");
		scanf("%d", &msg.flag);
		getchar();
		
		send(sockfd, &msg, sizeof(msg), 0);
		recv(sockfd, &msg, sizeof(msg), 0);
		printf("%s\n", msg.text);
	}else{
		do_add(sockfd, msg);
	}
}

//3.4 删除用户，通过界面信息提示用户输入选项，
//		将相应的内容发送给服务器，，等待接收服务器的反馈消息
void do_del(int sockfd, struct msg msg){
	msg.type = D;
	printf("请输入您要删除的工号:");
	scanf("%d", &msg.t_id);
	getchar();
	
	send(sockfd, &msg, sizeof(msg), 0);
	recv(sockfd, &msg, sizeof(msg), 0);
	printf("%s\n", msg.text);
}

//3.5 历史纪录查询，通过界面信息提示用户输入选项，
//		将相应的内容发送给服务器，循环等待接收服务器的反馈消息，直到反馈消息结束
void do_history(int sockfd, struct msg msg){
	msg.type = H;
	memset(msg.text, 0, sizeof(msg.text));
	
	send(sockfd, &msg, sizeof(msg), 0);
	while(strncmp(msg.text, "end", 3) != 0){
		recv(sockfd, &msg, sizeof(msg), 0);
		printf("%s\n", msg.text);
	}
}

//3. 管理员模式登陆的界面，按照提示信息输入所有需要的相关内容，
//	 并通过选项跳转到相应的功能函数
//	 选择退出直接跳转到上一级界面
void do_administrators_mode(int sockfd, struct msg msg){
	msg.type = R;
	printf("请输入你的名字 >>>");
	scanf("%s", msg.name);
	getchar();
	printf("请输入你的密码 >>>");
	scanf("%s", msg.passward);
	getchar();
	msg.flag = 1;

	send(sockfd, &msg, sizeof(msg), 0);
	recv(sockfd, &msg, sizeof(msg), 0);

	if(strncmp(msg.text,"ok",2) == 0){
		printf("亲爱的管理员，欢迎您登陆员工管理系统！\n");
		while(1){
			memset(msg.text, 0, sizeof(msg.text));
			int choice;

			printf("****************************************\n");
			printf("**1 查询      2 修改      3 添加用户****\n");
			printf("**4 删除用户  5 历史纪录  6 退出********\n");
			printf("****************************************\n");

			printf("请输入你的选择>>>");
			scanf("%d", &choice);
			getchar();

			switch(choice){
				case 1:
					do_query(sockfd, msg);
					break;
				case 2:
					do_modify(sockfd, msg);
					break;
				case 3:
					do_add(sockfd, msg);
					break;
				case 4:
					do_del(sockfd, msg);
					break;
				case 5:
					do_history(sockfd, msg);
					break;
				case 6:
					return;
			}
		}
	}else{
		printf("%s\n", msg.text);
	}
}

//4.1 普通用户查询自己信息，联系服务器的代码，相对应的发送消息，
//	  第一次发送查询功能选项，第二次发送自己的名字给服务器，获得服务器的反馈信息
//	  第三次发送一个停止位，让服务器的代码跳转到上一级
void do_query_normal(int sockfd, struct msg msg){
	msg.type = Q;
	send(sockfd, &msg, sizeof(msg), 0);
	recv(sockfd, &msg, sizeof(msg), 0);
	printf("%s\n", msg.text);
	
	msg.type = NAME;
	msg.flag = 0;
	strcpy(msg.text, msg.name);

	send(sockfd, &msg, sizeof(msg), 0);
	recv(sockfd, &msg, sizeof(msg), 0);
	printf("%s\n", msg.text);
	
	msg.type = T;
	send(sockfd, &msg, sizeof(msg), 0);
}

//4.2 普通用户修改自己的信息，按照提示信息输入所有需要的相关内容，
//	 并将选择发送给服务器，等待接收服务器的反馈消息
void do_modify_normal(int sockfd, struct msg msg){
	int choice = 0;
	
	while(1){
		msg.type = M;
		printf("****************************************************\n");
		printf("****************请输入你要修改的项目****************\n");
		printf("***1 年龄   2 家庭住址   3 电话   4 职位   5 退出***\n");
		printf("****************************************************\n");
		printf("请输入您的选择 >>>");
		scanf("%d", &choice);
		getchar();
		
		if(choice == 5){
			break;
		}

		printf("请输入新的内容 >>>");
		scanf("%s", msg.text);
		getchar();
		msg.choice = choice + 1;
		msg.t_id = msg.id;
		send(sockfd, &msg, sizeof(msg), 0);
		
		recv(sockfd, &msg, sizeof(msg), 0);
		printf("%s\n", msg.text);
	}
}

//4. 普通用户模式登陆，按照提示信息输入所有需要的相关内容，
//	 并通过选项跳转到相应的功能函数
//	 选择退出直接跳转到上一级界面
void do_normal_mode(int sockfd, struct msg msg){
	msg.type = O;
	printf("请输入你的名字 >>>");
	scanf("%s", msg.name);
	getchar();
	printf("请输入你的密码 >>>");
	scanf("%s", msg.passward);
	getchar();
	msg.flag = 0;

	send(sockfd, &msg, sizeof(msg), 0);
	recv(sockfd, &msg, sizeof(msg), 0);

	if(strncmp(msg.text,"ok",2) == 0){
		printf("亲爱的用户，欢迎您登陆员工管理系统！\n");
		while(1){
			memset(msg.text, 0, sizeof(msg.text));
			int choice;

			printf("************************************\n");
			printf("**1 查询      2 修改      3 退出****\n");
			printf("************************************\n");

			printf("请输入你的选择>>>");
			scanf("%d", &choice);
			getchar();

			switch(choice){
				case 1:
					do_query_normal(sockfd, msg);
					break;
				case 2:
					do_modify_normal(sockfd, msg);
					break;
				case 3:
					return;
			}
		}
	}else{
		printf("%s\n", msg.text);
	}
}

//
void do_quit(int sockfd, struct msg msg){
	msg.type = T;
	send(sockfd,&msg,sizeof(msg),0);
	recv(sockfd,&msg,sizeof(msg),0);
	if(strncmp(msg.text,"quit",4) == 0)
		printf("感谢您的使用\n");
}

// 2. 在主函数里首先通过TCP连接流程，
//    分别实现socket->填充->绑定->监听->等待连接->数据交互->关闭
int main(int argc, const char *argv[])
{	
	// 2.1 定义所需要的变量
	int choice;
	struct msg msg;
	int sockfd;
	int acceptfd;
	ssize_t recvbytes,sendbytes;
	char buf[N] = {0};
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	socklen_t ser_len = sizeof(serveraddr);
	socklen_t cli_len = sizeof(clientaddr);
	
	//2.2 创建网络通信的套接字
	sockfd = socket(AF_INET,SOCK_STREAM, 0);
	if(sockfd == -1){
		perror("socket failed.\n");
		exit(-1);
	}
	printf("sockfd :%d.\n",sockfd); 

	//2.3 填充网络结构体
	memset(&serveraddr, 0, sizeof(serveraddr));
	memset(&clientaddr, 0, sizeof(clientaddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port   = htons(atoi(argv[2]));
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);

	//2.4 等待连接服务器
	if(connect(sockfd, (const struct sockaddr *)&serveraddr, ser_len) == -1){
		perror("connect failed.\n");
		exit(-1);
	}

	//2.5 循环实现应用功能，当选择退出的时候，goto跳出循环，并关闭网络套接字socket
	while(1){
		printf("******************************************\n");
		printf("***1 管理员模式  2 普通用户模式  3 退出***\n");
		printf("******************************************\n");

		printf("请输入你的选择:");
		scanf("%d", &choice);
		getchar();

		switch(choice){
			case 1:
				do_administrators_mode(sockfd, msg);
				break;
			case 2:
				do_normal_mode(sockfd, msg);
				break;
			case 3:
				do_quit(sockfd, msg);
				goto END;
		}
	}
	return 0;

END:
	close(sockfd);
	return 0;
}

