#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

int sigint_flag = 0;

void handle_sigint(int sig) {
    printf("[srv] SIGINT is coming!\n");
    sigint_flag = 1;
}

void handle_sigpipe(int sig) {
    printf("[srv] SIGPIPE caught, ignoring...\n");
}

void calc_5(int connfd);

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <PORT>\n", argv[0]);   // 输入参数数量错误的处理
        exit(EXIT_FAILURE);
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);       // atoi函数将端口转化为int类型，存储服务器监听的IP和端口

    struct sigaction sa_int;
    sa_int.sa_flags = 0;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sigaction(SIGINT, &sa_int, NULL); // 注册SIGINT信号处理器

    struct sigaction sa_pipe;
    sa_pipe.sa_flags = 0;
    sa_pipe.sa_handler = SIG_IGN;
    sigemptyset(&sa_pipe.sa_mask);
    sigaction(SIGPIPE, &sa_pipe, NULL); // 忽略SIGPIPE信号


    struct sockaddr_in servaddr;      //创建一个套接字地址的结构体
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = htons(port);


/* ----------- Socket核心系统 ------------- */
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);                 // 创建套接字
    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));  // 将套接字和地址绑定起来
    listen(listenfd, 5);                                            // 设置监听模式
/* --------------------------------------- */

    printf("[srv] server[%s:%d] is initializing!\n", ip, port);     // 输出服务器 IP 和端口号

    while (!sigint_flag) {
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);
        int connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
        if (connfd < 0) {                      // 对中断信号的处理
            if (errno == EINTR) {
                continue;                      // 被信号中断，重新执行accept
            } else {
                perror("accept failed");
                break;
            }
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cliaddr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("[srv] client[%s:%d] is accepted!\n", client_ip, ntohs(cliaddr.sin_port));    // 连接成功，输出客户端 IP 和端口号

        calc_5(connfd);  // 执行自五则运算的业务
        close(connfd);   // 关闭对接客户端业务的套接字

        printf("[srv] client[%s:%d] is closed!\n", client_ip, ntohs(cliaddr.sin_port));      // 断开连接，输出客户端 IP 和端口号
    }

    close(listenfd);                       // 关闭受理客户端请求的套接字
    printf("[srv] listenfd is closed!\n");

    printf("[srv] server is going to exit!\n");

    return 0;
}

void calc_5(int connfd) {     // 将套接字参数传入业务处理函数中
    while (1) {
        struct {
            int32_t op;
            int64_t op1;
            int64_t op2;
        } req;                // 定义运算式，左右操作数为64位有符号整型，操作符为32位有符号整型

        ssize_t size_read = read(connfd, &req, sizeof(req));
        if (size_read == 0) return ;           // 客户端正常结束，业务处理完毕，返回大循环

        int32_t op = ntohl(req.op);
        int64_t op1 = be64toh(req.op1);
        int64_t op2 = be64toh(req.op2);    // 字节序转换

        int64_t result;  // 存结果和运算符
        char op_c;

        switch (op) {
            case 1: // ADD
                result = op1 + op2;
                op_c = '+';
                break;
            case 2: // SUB
                result = op1 - op2;
                op_c = '-';
                break;
            case 3: // MUL
                result = op1 * op2;
                op_c = '*';
                break;
            case 4: // DIV
                if (op2 == 0) {
                    fprintf(stderr, "Division by zero\n");    // 特判
                    result = 0;
                } else {
                    result = op1 / op2;
                }
                op_c = '/';
                break;
            case 5: // MOD
                if (op2 == 0) {
                    fprintf(stderr, "Modulo by zero\n");      // 特判
                    result = 0;
                } else {
                    result = op1 % op2;
                }
                op_c = '%';
                break;
            default:
                fprintf(stderr, "Unknown operation\n");
                result = 0;
                break;
        }

        printf("[rqt_res] %ld %c %ld = %ld\n", op1, op_c, op2, result);

        write(connfd, &result, sizeof(result));
    }
}
