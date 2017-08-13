#include <stdio.h> 
#include "telnetclient.h"
#include "unistd.h"
 
#define MAX_PARAMTER 3  //参数个数 
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
    //fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK); //设置SOCKET文件为非阻塞方式  
    //fcntl(STD_IN, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK); //设置标准输入文件为非阻塞方式  
    while (telnet_connect)  //当将sockfd和标准输入输出STD_IN设置为非阻塞后，如果一方收到数据则继续接收，如果两方都收不到数据则效果为等待从键盘输入数据何等待接收  
    { 
        if(cs_communcate() < 0)  //处理服务发来的数据  
        {  
            close(sockfd);  
            return -1;  
        }  
    }  
  
    return 0;  
}  


