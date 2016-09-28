#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "reactor.h"

#define MAX_EVENT_NUMBER 10240
#define BUFFER_SIZE 10
#define DEFAULT_EVENT_BUFFER_SIZE 10240

char g_recvDataBuf[BUFFER_SIZE];

typedef struct sockEvent {
    struct sockEvent* next;
    int sock;                               // socket号
    int woffset;                            // buffer写入偏移
    int roffset;                            // buffer读取偏移
    int remaining;                          // 还未读取的长度
    int headMark;                          // 通信协议头已接收标志
    int recvTimes;                         // 包接收完的读取次数，测试用
    comHead_t head;                        // 通信协议头
    char* wp;                               // buffer写入指针
    char buffer[DEFAULT_EVENT_BUFFER_SIZE]; // 默认接收buffer
    int bigBufSize;                       // bigbuffer的大小
    char* bigBuffer;                        // 数据长度超过默认buffer时，使用动态申请的大buffer
} sockEvent_t;

typedef struct eventList {
    sockEvent_t* head;
    sockEvent_t* tail;
} eventList_t;

eventList_t evListHead;

void (*processFunPointer)(int sock, char* dataReceived, int receivedLen);

void setProcessHandle(void (*fun)(int, char*, int))
{
    processFunPointer = fun;
}

// List
/*
typedef event_list {
    sock_event_t* head;
    sock_event_t* tail;
} event_list_t;
*/

void freeSockEvent(sockEvent_t* ev)
{
    if(ev->bigBuffer != NULL) {
        //printf("free big_buffer:%d\n", ev->sock);
        free(ev->bigBuffer);
    }

    free(ev);
    ev = NULL;
}

int addList(eventList_t* head, sockEvent_t* ev)
{
    if (head->head == NULL && head->tail == NULL) {
        head->head = ev;
        head->tail = ev;
    } else if(head->head != NULL) {
        head->tail->next = ev;
        head->tail = ev;
    } else {
        return -1;   
    }    

    return 0;
}

sockEvent_t* findList(eventList_t* head, int sock)
{
    sockEvent_t* se = head->head;
    while(se != NULL) {
        if(se->sock == sock) {
            break;
        }

        se = se->next;
    }

    return se;
}

int removeList(eventList_t* head, sockEvent_t* ev)
{
    printf("remove sock(%d)\n", ev->sock);
    sockEvent_t* iter = head->head;
    if(iter == ev) {
        head->head = ev->next; //is NULL
        if(ev == head->tail) {
            head->tail = ev->next; //is NULL
        }
        freeSockEvent(ev);
    } else {
        while(iter != NULL) {
            if(iter->next == ev) {
                if(ev == head->tail) {
                    head->tail = iter;
                }
                iter->next = ev->next;
                freeSockEvent(ev);
                break;
            }

            iter = iter->next;
        }
    }

    return 0;
}

int removeListBySocket(eventList_t* head, int sock)
{
    sockEvent_t* iter = head->head;
    sockEvent_t* ev = findList(head, sock);
    
    removeList(head, ev);
}

void freeList(eventList_t* head)
{
    sockEvent_t* iter = head->head;
    sockEvent_t* tmp = NULL;

    while(iter != NULL) {
        tmp = iter->next;
        freeSockEvent(iter);
        iter = tmp;
    }

    head->head = NULL;
    head->tail = NULL;
}
     

int setNonBlocking(int fd)
{
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);

    return oldOption;
}

void addFd(int epollFd, int fd, int enableET)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if(enableET == 1) {
        event.events |= EPOLLET;
    }
    setNonBlocking(fd);

    epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
    
}



#define READ_OK    0
#define READ_ERR   -1
#define READ_LATER -2
#define READ_BAD   -3

int doRead(sockEvent_t* ev)
{
    int ret = 0;

    if(ev->headMark == FALSE) {
        ret = recv(ev->sock, (char*)&ev->head, sizeof(comHead_t), 0);
        if(ret != sizeof(comHead_t)) {
            if((ret < 0) && (errno == EAGAIN || errno != EWOULDBLOCK)) {
                return READ_LATER;
            }
    
            close(ev->sock);
            removeList(&evListHead, ev);
            return READ_ERR;
        }

        if(ev->head.headMagic != HEAD_MAGIC) {
            close(ev->sock);
            removeList(&evListHead, ev);
            return READ_ERR;
        }

        ev->headMark = TRUE;
        ev->remaining = ev->head.dataLen;
        if(ev->head.dataLen > DEFAULT_EVENT_BUFFER_SIZE) {
            ev->bigBuffer = (char*)malloc(ev->head.dataLen);
            if(ev->bigBuffer == NULL) {
                close(ev->sock);
                removeList(&evListHead, ev);
                return READ_ERR;
            }
            ev->wp = ev->bigBuffer;
            ev->bigBufSize = ev->head.dataLen;
        } else {
            ev->wp = ev->buffer;
        }
    }
        
    ev->recvTimes++;
    ret = recv(ev->sock, ev->wp, ev->remaining, 0);
    if(ret < 0) {
        if(errno != EAGAIN && errno != EWOULDBLOCK) {
            close(ev->sock);
            removeList(&evListHead, ev);
            return READ_ERR;
        } else {
            return READ_LATER;
        }
    } else {
        if(ret != ev->remaining) {
            ev->remaining -= ret;
            ev->wp += ret;
        } else {
            ev->headMark = FALSE;
    
            printf("(%d)get (%d) bytes of content:%s\n", ev->recvTimes, ev->head.dataLen, ev->head.dataLen >  DEFAULT_EVENT_BUFFER_SIZE ? ev->bigBuffer : ev->buffer);
            ev->recvTimes = 0;
            //do something meaningful
            if(processFunPointer != NULL) {
                (*processFunPointer)(ev->sock, 
                                     ev->head.dataLen > DEFAULT_EVENT_BUFFER_SIZE ? ev->bigBuffer : ev->buffer, 
                                     ev->head.dataLen);
                //send(ev->sock, response, strlen(response), 0);
            }
        }
    }

    return READ_OK;
}

void doProcess(int sockFd, int isET)
{
    printf("event trigger once\n");
    int ret = 0;
    sockEvent_t* ev = findList(&evListHead, sockFd);
    if(ev == NULL) {
        printf("session of socket %d not existed\n", sockFd);
        //sleep(1);
        return;
    }

    if(isET == FALSE) {
        doRead(ev);
    } else {
        while(1) {
            ret = doRead(ev);
            if(ret == READ_LATER || ret == READ_ERR) break;
            /*
            ret = recv(sockfd, (char*)&ev->head, sizeof(com_head_t), 0);
            if(ret <= 0) {
                if((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    printf("read later\n");
                    break;
                }
                close(sockfd);
                break;
            } else if(ret == 0) {
                close(sockfd);
            } else {
                if(ev->head.head_magic == HEAD_MAGIC) {
                    ret = recv(sockfd, g_recv_data_buf, ev->head.data_len, 0);

                    printf("get %d bytes of content:%s\n", ret, g_recv_data_buf);
                    //do something meaningful
                } else {
                    close(sockfd);
                    return;
                }
            }*/
        }
    }
}

void lt(struct epoll_event* events, int number, int epollFd, int listenFd)
{
    int i;
    char buf[BUFFER_SIZE];
    for(i = 0; i < number; i++) {
        int sockFd = events[i].data.fd;
        if(sockFd == listenFd) {
            struct sockaddr_in clientAddress;
            socklen_t clientAddrLength = sizeof(clientAddress);
            int connFd = accept(listenFd, 
                                (struct sockaddr*)&clientAddress, 
                                &clientAddrLength);

            sockEvent_t* sockEvent = (sockEvent_t*)malloc(sizeof(sockEvent_t));
            if(sockEvent != NULL) {
                memset((char*)sockEvent, 0, sizeof(sockEvent_t));
                sockEvent->sock = connFd;
                addList(&evListHead, sockEvent);
                addFd(epollFd, connFd, FALSE);
            }
        } else if(events[i].events & EPOLLIN) {
            doProcess(sockFd, FALSE);
        } else {
            printf("something else happened\n");
        }
    }
}

void et(struct epoll_event* events, int number, int epollFd, int listenFd)
{
    int i;
    char buf[BUFFER_SIZE];

    for(i = 0; i < number; i++) {
        int sockFd = events[i].data.fd;
        if(sockFd == listenFd) {
            struct sockaddr_in clientAddress;
            socklen_t clientAddrLength = sizeof(clientAddress);
            int connFd = accept(listenFd, 
                                (struct sockaddr*)&clientAddress, 
                                &clientAddrLength);

            sockEvent_t* sockEvent = (sockEvent_t*)malloc(sizeof(sockEvent_t));
            if(sockEvent != NULL) {
                memset((char*)sockEvent, 0, sizeof(sockEvent_t));
                sockEvent->sock = connFd;
                addList(&evListHead, sockEvent);
                addFd(epollFd, connFd, TRUE);
            }
        } else if(events[i].events & EPOLLIN) {
            doProcess(sockFd, TRUE);
        } else {
            printf("something else happened\n");
        }
    }
}

int startReactor(int setET, int port)
{
    int ret = 0;
    struct sockaddr_in address;

    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenFd >= 0);

    ret = bind(listenFd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenFd, 5);
    assert(ret != -1);

    struct epoll_event events[MAX_EVENT_NUMBER];
    int epollFd = epoll_create(5);
    assert(epollFd != -1);
    addFd(epollFd, listenFd, TRUE);

    while(1) {
        ret = epoll_wait(epollFd, events, MAX_EVENT_NUMBER, -1);
        if(ret < 0) {
            printf("epoll failure\n");
            break;
        }

        if(setET == TRUE) {
            et(events, ret, epollFd, listenFd);
        } else {
            lt(events, ret, epollFd, listenFd);
        }
    }
    
    close(listenFd);

    return 0;
}



//测试代码

#ifdef TEST_FULL

int main(void)
{
    startReactor(TRUE, 8421);

    return 0;
}

#elif TEST_LIST

void getOne(int sock)
{
    sockEvent_t* ev = NULL;
    printf("get sock:%d, info:\n", sock);
    ev = findList(&evListHead, sock);
    if(ev != NULL) {
        printf("buffer info:%s\n", ev->buffer);
    } else {
        printf("not existed\n");
    }
}
/*
int main(void)
{
    int sock = 1;
    sockEvent_t* ev = NULL;

    printf("Create 10 sock list...\n");
    while(sock < 10) {
        ev = (sockEvent_t*)malloc(sizeof(sockEvent_t));
        memset(ev, 0, sizeof(sockEvent_t));

        ev->sock = sock;
        sprintf(ev->buffer, "this is node %d", sock);

        if(sock == 6 ||sock == 8) {
            ev->bigBuffer = (char*)malloc(20480);
        }

        addList(&evListHead, ev);
        sock++;
    }

    getOne(2);
    getOne(3);
    getOne(5);
    getOne(8);
    getOne(9);

    removeListBySocket(&evListHead, 8);
    removeListBySocket(&evListHead, 1);

    getOne(1);
    getOne(8);

    freeList(&evListHead);

    return 0;
}*/

int main(void)
{
    int sock = 5;
    sockEvent_t* ev = NULL;

    printf("Create sock list...\n");
        ev = (sockEvent_t*)malloc(sizeof(sockEvent_t));
        memset(ev, 0, sizeof(sockEvent_t));

        ev->sock = sock;
        sprintf(ev->buffer, "this is node %d", sock);

        addList(&evListHead, ev);
        getOne(5);

        removeListBySocket(&evListHead, 5);


        ev = (sockEvent_t*)malloc(sizeof(sockEvent_t));
        memset(ev, 0, sizeof(sockEvent_t));

        ev->sock = sock;
        sprintf(ev->buffer, "this is node %d", sock);

        addList(&evListHead, ev);

        getOne(5);


    freeList(&evListHead);

    return 0;
}

#endif

