#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void calc_5(int connfd);

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <PORT>\n", argv[0]);     // 输入参数数量错误的处理
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);


    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &servaddr.sin_addr);

/* ---------- 初始化两步 -------- */
    int connfd = socket(AF_INET, SOCK_STREAM, 0); // 创建套接字
    connect(connfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
/* ----------------------------- */

    printf("[cli] server[%s:%d] is connected!\n", server_ip, server_port);

    calc_5(connfd); // 执行业务逻辑，与服务器进行收发互动

    close(connfd);                           // 关闭连接服务器的套接字
    printf("[cli] connfd is closed!\n");

    printf("[cli] client is going to exit!\n");
    return 0;
}

void calc_5(int connfd) {
    char buffer[128];
    while (1) {
        fgets(buffer, sizeof(buffer), stdin);   // 从标准输入中读一行字符存入buffer

        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "EXIT") == 0) {       // 读入到EXIT指令
            printf("[cli] command EXIT received\n");
            return ;
        }

        // 解析用户输入
        char op_str[4];
        int64_t op1, op2;
        if (sscanf(buffer, "%s %ld %ld", op_str, &op1, &op2) != 3) {
            fprintf(stderr, "Invalid input format. Expected format: {ADD | SUB | MUL | DIV | MOD} <OP1> <OP2>\n");
            continue;
        }

        int32_t op;
        char op_c;
        if      (strcmp(op_str, "ADD") == 0) op = 1, op_c = '+';
        else if (strcmp(op_str, "SUB") == 0) op = 2, op_c = '-';
        else if (strcmp(op_str, "MUL") == 0) op = 3, op_c = '*';
        else if (strcmp(op_str, "DIV") == 0) op = 4, op_c = '/';
        else if (strcmp(op_str, "MOD") == 0) op = 5, op_c = '%';
        else {
            fprintf(stderr, "Unknown operation: %s\n", op_str);
            continue;
        }

        // 构建请求PDU
        struct {
            int32_t op;
            int64_t op1;
            int64_t op2;
        } req;         // 定义运算式，左右操作数为64位有符号整型，操作符为32位有符号整型

        req.op = htonl(op);
        req.op1 = htobe64(op1);
        req.op2 = htobe64(op2);    // 字节序转换

        int64_t result;

        ssize_t size_write = write(connfd, &req, sizeof(req));        // 发送请求PDU
        ssize_t size_read = read(connfd, &result, sizeof(result));    // 接收响应PDU

        printf("[rep_rcv] %ld %c %ld = %ld\n", op1, op_c, op2, result);
    }
}
