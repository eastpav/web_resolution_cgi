#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "cgic.h"
#include "parson.h"
#include "ipc.h"

char html_buf[10240];

char requestGetString[] = "{\"cmd\":\"NET_CONF\", \"method\":\"GET\"}";
char requestSetString[] = "{\"cmd\":\"NET_CONF\", \"method\":\"SET\",\"serverIp\":%s, \"serverPort\":%d, \"interval\":%d}";

int cgiMain() 
{
    FILE* fhtm = NULL;
    char* resp = NULL;
    int respLen = 0;
    JSON_Value* jVaule = NULL;
    JSON_Object* jsonObj = NULL;
    int port = -1;
    int interval = -1;

    cgiHeaderContentType("text/html");

    fhtm = fopen("../net.htm", "r");
    if(fhtm == NULL) return -1;

    fread(html_buf, 1, 10240, fhtm);

    fclose(fhtm);

    int sock = attachToHost("127.0.0.1", 8421);

    if(strcmp(cgiRequestMethod, "GET") == 0) {
        char* ip = NULL;
        sendPacket(sock, requestGetString);
        recvResponse(sock, &resp, &respLen);

        jVaule = json_parse_string(resp);   
        jsonObj = json_object(jVaule);

        ip = (char*)json_object_get_string(jsonObj, "serverIp");
        if(json_object_has_value(jsonObj, "serverPort")) {
            port = (int)json_object_get_number(jsonObj, "serverPort");
        }
        if(json_object_has_value(jsonObj, "interval")) {
            interval = (int)json_object_get_number(jsonObj, "interval");
        }
        
        fprintf(cgiOut, html_buf, ip, port, interval);
        json_value_free(jVaule);

    } else if(strcmp(cgiRequestMethod, "POST") == 0) {
        char server_address[20];
        cgiFormString("server_address", server_address, 20);
        cgiFormInteger("server_port", &port, -1);
        cgiFormInteger("server_interval", &interval, -1);

        char sendBuf[100];
        sprintf(sendBuf, requestSetString, server_address, port, interval);
        sendPacket(sock, sendBuf);
        recvResponse(sock, &resp, &respLen);
        
        fprintf(cgiOut, html_buf, server_address, port, interval);
    }
    
    return 0;    
}
