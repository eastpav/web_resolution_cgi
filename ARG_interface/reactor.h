#ifndef __REACTOR_H__
#define __REACTOR_H__

#define TRUE 1
#define FALSE 0

#define HEAD_MAGIC 0x4a434844 /* JCHD */

typedef struct comHead {
    int headMagic;
    int dataLen;
} comHead_t;

void setProcessHandle(void (*fun)(int, char*, int));
int startReactor(int setET, int port);

#endif

