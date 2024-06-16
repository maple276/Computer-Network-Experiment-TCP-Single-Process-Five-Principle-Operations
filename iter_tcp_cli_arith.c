#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <endian.h>

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

void build_pdu(uint8_t *buffer, int32_t op, int64_t op1, int64_t op2) {
    int32_t net_op = htonl(op);
    int64_t net_op1 = htobe64(op1);
    int64_t net_op2 = htobe64(op2);

    memcpy(buffer, &net_op, sizeof(net_op));
    memcpy(buffer + sizeof(net_op), &net_op1, sizeof(net_op1));
    memcpy(buffer + sizeof(net_op) + sizeof(net_op1), &net_op2, sizeof(net_op2));
}

void parse_pdu(const uint8_t *buffer, int32_t *op, int64_t *op1, int64_t *op2) {
    memcpy(op, buffer, sizeof(*op));
    memcpy(op1, buffer + sizeof(*op), sizeof(*op1));
    memcpy(op2, buffer + sizeof(*op) + sizeof(*op1), sizeof(*op2));

    *op = ntohl(*op);
    *op1 = be64toh(*op1);
    *op2 = be64toh(*op2);
}

void calc_5(int connfd) {
    char buffer[128];
    while (1) {
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "EXIT") == 0) {
            printf("[cli] command EXIT received\n");
            return;
        }

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
        else if (strcmp(op_str, "MUL") == 0) op = 4, op_c = '*';
        else if (strcmp(op_str, "DIV") == 0) op = 8, op_c = '/';
        else if (strcmp(op_str, "MOD") == 0) op = 16, op_c = '%';
        else {
            fprintf(stderr, "Unknown operation: %s\n", op_str);
            continue;
        }

        uint8_t pdu[sizeof(int32_t) + 2 * sizeof(int64_t)];
        build_pdu(pdu, op, op1, op2);

        int64_t result;

        ssize_t size_write = write(connfd, pdu, sizeof(pdu));
        if (size_write != sizeof(pdu)) {
            fprintf(stderr, "Failed to send complete PDU\n");
            continue;
        }

        ssize_t size_read = read(connfd, &result, sizeof(result));
        if (size_read != sizeof(result)) {
            fprintf(stderr, "Failed to read complete result\n");
            continue;
        }

        result = be64toh(result);

        printf("[rep_rcv] %ld %c %ld = %ld\n", op1, op_c, op2, result);
    }
}
