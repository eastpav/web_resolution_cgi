#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include "cgic.h"
#include "parson.h"
#include "ipc.h"

char html_buf[10240];

char requestString[] = "{\"cmd\":\"BASE_INFO\", \"method\":\"GET\"}";

int getMac(char* mac)
{
    struct ifreq tmp;
    int sock_mac;
    char mac_addr[30];
    sock_mac = socket(AF_INET, SOCK_STREAM, 0);
    if( sock_mac == -1)
    {
        perror("create socket fail\n");
        return -1;
    }
    memset(&tmp,0,sizeof(tmp));
    strncpy(tmp.ifr_name,"eno16777736",sizeof(tmp.ifr_name)-1 );
    if( (ioctl( sock_mac, SIOCGIFHWADDR, &tmp)) < 0 )
    {
        printf("mac ioctl error\n");
        return -1;
    }
    sprintf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
            (unsigned char)tmp.ifr_hwaddr.sa_data[0],
            (unsigned char)tmp.ifr_hwaddr.sa_data[1],
            (unsigned char)tmp.ifr_hwaddr.sa_data[2],
            (unsigned char)tmp.ifr_hwaddr.sa_data[3],
            (unsigned char)tmp.ifr_hwaddr.sa_data[4],
            (unsigned char)tmp.ifr_hwaddr.sa_data[5]
            );
    close(sock_mac);
    strcpy(mac, mac_addr);
    return 0;
}

int cgiMain() 
{
    FILE* fhtm = NULL;
    char* resp = NULL;
    int respLen = 0;
    JSON_Value* jVaule = NULL;
    JSON_Object* baseInfo = NULL;

    char* devId = NULL;

    cgiHeaderContentType("text/html");

    fhtm = fopen("../base.htm", "r");
    if(fhtm == NULL) return -1;

    fread(html_buf, 1, 10240, fhtm);

    fclose(fhtm);

    int sock = attachToHost("127.0.0.1", 8421);
    sendPacket(sock, requestString);
    recvResponse(sock, &resp, &respLen); 

    jVaule = json_parse_string(resp);   
    baseInfo = json_object(jVaule);

    devId = (char*)json_object_get_string(baseInfo, "devId");
    if(devId == NULL) {
        devId = "Unknown";
    }
    
    char* mac[30];
    getMac(mac);

    fprintf(cgiOut, html_buf, devId, mac);
    free(resp);
    json_value_free(jVaule);
    
    return 0;    
}
