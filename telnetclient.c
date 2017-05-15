#include <iostream>
#include <sys/types.h>  
#include <unistd.h>  
#include <sys/socket.h>  
#include <string.h>  
#include <netinet/in.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <arpa/inet.h>  
#include <fcntl.h>  
#include <errno.h>  

using namespace std;  
#define MAX_PARAMTER 3  //参数个数  
/*TELNET命令*/  
#define IAC 255    //TELNET协商语句以此开头  
#define WILL 251   //请求本方动作，应答同意要求本方  
#define WONT 252   //请求本方不动作，应答不同意本方动作或同意本方不动作  
#define DO   253   //请求对方动作，应答同意对方要求动作  
#define DONT 254   //请求对方不动作，应答不同意对方要求动作或同意对方不动作  
/*TELNET选项*/  
#define ECHO 1      //TELNET回显选项，TELNET协商语句以此为最后一部分（子协议除外）  
  
#define NEGONATION_LENTH 3 //TELNET协商语句长度  
  
#define MAX_COMMAND 1024  //终端输入最大长度  
#define MAX_RECEIVE 1024  //接收最大长度  
#define MAX_SEND    1024  //发送最大长度  
  
/*用于区别是发来的请求还是应答（由于请求应答相同有对称性）*/  
#define SERVER_ECHO_ON  1 //已成为服务器回显状态  
#define SERVER_ECHO_OFF 0 //还未成服务器为回显状态  
  
#define STD_IN 0  //标准输入输出文件描述符  
#define STD_OUT 1  //标准输入输出文件描述符  
  
unsigned char echo_status = 0; //回显状态  
int sockfd = -1;  //SOCKET文件描述  
int telnet_connect = 0; //TELNET通断控制变量  
  
int init_socket(char* server_ip, char * server_port)  
{  
    sockfd = socket(AF_INET, SOCK_STREAM, 0); //参数1为IPV4，参数2为TCP链接，参数3固定为0  
      
    if (sockfd < 0)  
    {  
        printf("Socket error\n");  
        return -1;  
    }  
  
    struct sockaddr_in server_addr;  //服务器地址信息  
      
    bzero(&server_addr, sizeof(server_addr));  //清空server_addr  
  
    server_addr.sin_family = AF_INET;            //IPV4  
    server_addr.sin_addr.s_addr = inet_addr(server_ip);     //服务器IP地址  
    server_addr.sin_port = htons((unsigned short)(atoi(server_port)));   //服务器端口  
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)   //与服务器取得链接  
    {  
        printf("Connect error\n");  
        return -1;  
    }  
  
    telnet_connect = 1;  
    return 0;  
}  
  
int deal_telnet_protocol(unsigned char* receive_server)  
{  
    unsigned char client_send[NEGONATION_LENTH];  
    int i = 0;  
  
    client_send[0] = receive_server[0];   //IAC  
    client_send[2] = receive_server[2];   //服务器要求的选项  
  
    switch (receive_server[1])  
    {  
        case WILL:                                //如果收到服务器要求改变状态  
            {  
                if (receive_server[2] == ECHO)     //查看选项是否为回显  
                {  
                    if (echo_status == SERVER_ECHO_OFF)   //查看现在的服务器ECHO状态，通过使用状态位判断服务器发来的为请求还是应当，因为telnet的请求命令和应答都相同有对称性（由于客户端没发生过请求，服务器端也就不会发应答，发来的数据都是请求，其实不用判断）  
                    {                                    //如果现在是服务器回显状态，则ECHO的状态没有改变，不发送应答，如果不是服务器回显状态，则ECHO的状态改变了，发送应答  
                        client_send[1] = DO;             //同意服务器端开启ECHO选项  
                        echo_status = SERVER_ECHO_ON;  //修改服务器ECHO状态为ON  
                    }  
                    else  
                    {  
                        return 0;                 //否则不应答  
                    }  
                }  
                else                              //其他服务器端选项都不同意，因为客户端从没有向服务器发送命令，  
                {                                 //因此服务器发来的数据不可能是应答，都是请求，因此其实不需要判断状态  
                    client_send[1] = DONT;          
                }  
                break;  
            }  
        case WONT:  
            {  
                if (receive_server[2] == ECHO)  //查看选项是否为回显  
                {  
                    if (echo_status == SERVER_ECHO_ON) //现在状态是否为ON  
                    {  
                        client_send[1] = DONT;  
                        echo_status = SERVER_ECHO_OFF;  
                    }  
                    else  
                    {  
                        return 0;               //否则不应答  
                    }  
  
                }  
                else                           //WONT如果要回复只能用DONT  
                {  
                    client_send[1] = DONT;           
                }  
                break;  
            }  
        case DO:                               //其余的要求客户端做的选项都拒绝  
            {  
                client_send[1] = WONT;           
                break;  
            }  
        case DONT:                             //DONT选项只能用WONT来应答  
            {  
                client_send[1] = WONT;           
                break;  
            }  
        default:  
            {  
                printf("Receive error telnet command\n");  
                return -1;  
            }  
    }  
      
    if (send(sockfd, client_send, NEGONATION_LENTH, 0) < 0) //发送应答数据包  
    {  
        printf("send ack error\n");  
        return -1;  
    }  
    return 0;  
}  
  
int  cs_communcate(void)  
{  
    int re_recv = -1;  
    int i = 0;  
    unsigned char* p_server = NULL; //指向收到服务器数据的指针  
  
    unsigned char server_data[MAX_RECEIVE];  //接收到的服务数据  
    unsigned char server_negonation[NEGONATION_LENTH];  
      
    re_recv = recv(sockfd, server_data, MAX_RECEIVE, 0); //接收服务器发来的数据  
    if (re_recv > 0)  
    { 
        int deal_lenth = 0;  
        p_server = server_data;  
        while (deal_lenth < re_recv)  
        {  
            if ((*p_server == IAC) && (*(p_server + 1) != IAC) )   //判断接收到的是否为协商语句（请求或应答，实际上因client未发送请求所以不可能为应答）  
            {  
                memcpy(server_negonation, p_server, NEGONATION_LENTH); //缓存协商语句   
                deal_telnet_protocol(server_negonation);  //处理每条服务器发来的协商语句  
  
                p_server += NEGONATION_LENTH;  
                deal_lenth += NEGONATION_LENTH;  
            }  
            else                                                 //如果是服务器发来的数据则把它显示到终端上  
            {  
                 cout << p_server;
                //write(STD_IN, p_server, strlen(p_server) + 1);  
                deal_lenth = re_recv;                            //认为收到的如果为数据则它在一包的最后，或者独立为一包数据  
            }  
        }     
  
    }  
  
    return 0;  
      
}  
  
int send_usrcommand()  
{  
    int i = 0;  
    char usr_command[MAX_COMMAND];  //终端发送的用户命令（不是TELNET协议中的命令）  
  
    if (read(STD_IN, usr_command, MAX_COMMAND) > 0) //如果读取用户命令成功则将其发送给服务器  
    {  
        fflush(stdout);  //清空输出缓冲区的内容，这样用户可以一次发送带空格的命令  
  
        if (!strcmp(usr_command, "exit\n"))    //如果输入字符串为exit则断开TELNET控制连接  
        {  
                telnet_connect = 0;  
                close(sockfd);  
                return 0;  
        }  
  
        if (send(sockfd, usr_command, strlen(usr_command) - 1, 0) < 0) //发送命令但不发送回车符  
        {  
            printf("Send usr command error\n");  
            return -1;  
        }  
        if (send(sockfd, "\r\n", 2, 0) < 0)   //NVT中以换行回车作为一个命令的结束（NVT用于在TELNET中连接不同操作系统,nvtasc码的转换由操作系统实现）  
        {  
            printf("Send usr command error\n");  
            return -1;  
        }  
          
    }  
  
    return 0;  
}  
  
int main(int argc, char** argv)  
{  
    if (argc != MAX_PARAMTER)  
    {  
        printf("2 parameters please\n");  
        return -1;  
    }  
  
    if (init_socket(argv[1], argv[2]) < 0)  
    {  
        return -1;  
    }  
     cs_communcate(); 
    //fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK); //设置SOCKET文件为非阻塞方式  
    //fcntl(STD_IN, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK); //设置标准输入文件为非阻塞方式  
    while (telnet_connect)  //当将sockfd和标准输入输出STD_IN设置为非阻塞后，如果一方收到数据则继续接收，如果两方都收不到数据则效果为等待从键盘输入数据何等待接收  
    { 
        if(cs_communcate() < 0)  //处理服务发来的数据  
        {  
            close(sockfd);  
            return -1;  
        }  
        if (send_usrcommand() < 0) //处理终端用户发来的命令  
        {  
            close(sockfd);  
            return -1;  
        }  
    }  
  
    return 0;  
}  

