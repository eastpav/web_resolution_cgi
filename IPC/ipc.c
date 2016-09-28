#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct comHead {
    int headMagic;
    int dataLen;
} comHead_t;

#define HEAD_MAGIC 0x4a434844 /* JCHD */

#if 0
char send_buffer[10240];

int send_packet(int sock, char* msg)
{
    com_head_t* head = (com_head_t*)send_buffer;
    int ret = 0;

    head->head_magic = HEAD_MAGIC;
    head->data_len = strlen(msg);
    char* pmsg = send_buffer + sizeof(com_head_t);
    strcpy(pmsg, msg);

    int send_len = head->data_len + sizeof(com_head_t);
    ret = send(sock, send_buffer, send_len, 0);
    if(ret != send_len) {
        return -1;
    }

    return 0;
}
#endif

int sendPacket(int sock, char* msg)
{
    comHead_t head;
    int ret = 0;

    head.headMagic = HEAD_MAGIC;
    head.dataLen = strlen(msg);
    ret = send(sock, (char*)&head, sizeof(comHead_t), 0);
    if(ret == sizeof(comHead_t)) {
        ret = send(sock, msg, head.dataLen, 0);
        if(ret != head.dataLen) {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}


int recvResponse(int sock, char** dataReceived, int* receivedLen)
{
    comHead_t head;
    int ret = 0;

    ret = recv(sock, (char*)&head, sizeof(head), 0);
    if(ret == sizeof(head)) {
        if(head.headMagic == HEAD_MAGIC) {
            *dataReceived = (char*)malloc(head.dataLen);
            if(*dataReceived != NULL) {
                recv(sock, *dataReceived, head.dataLen, 0);
                *receivedLen = head.dataLen;
            } else {
                return -1;
            }
        } else {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}

int attachToHost(char* ip, int port)
{
    int sock = 0;
    int ret = -1;
    struct sockaddr_in address;

    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    ret = connect(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret == 0);

    return sock;
}

int detach(int sock)
{
    close(sock);

    return 0;
}


//测试代码
#ifdef IPC_TEST

#define HOST "192.168.88.138"
#define PORT 8421

char testMsg1[] = "{\"cmd\":\"reset\", \"gid\":\"xxxxxxxx\", \"result\":\"ok\"}";
char testMsg2[] = "{\"cmd\":\"base_info\", \"gid\":\"xxxxxxxx\", \"result\":\"ok\"}";
char testMsg3[] = "{\"cmd\":\"net_cconf\", \"gid\":\"xxxxxxxx\", \"result\":\"ok\"}";

int main(void)
{
    int sock = attachToHost(HOST, PORT);
    assert(sock >= 0);
    
    int cnt = 3;
    char* msg = NULL;
    char* resp = NULL;
    int respLen = 0;

    while(cnt--) {
        if(cnt == 2) {
            msg = testMsg1;
        } else if(cnt == 1) {
            msg = testMsg2;
        } else {
            msg = testMsg3;
        }        
    
        sendPacket(sock, msg);
        recvResponse(sock, &resp, &respLen);
        printf("(%d btyes)%s\n", respLen, resp);
        free(resp);
    }

    detach(sock);

    return 0;
}
#endif

