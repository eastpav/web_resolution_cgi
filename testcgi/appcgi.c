#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "cgic.h"
#include "parson.h"
#include "ipc.h"

char html_buf[10240];

char requestGetString[] = "{\"cmd\":\"APP_CONF\", \"method\":\"GET\"}";
char requestSetString[] = "{\"cmd\":\"APP_CONF\", \"method\":\"SET\",\"heartbeatPeriod\":%d,\"reportPeriod\":%d,\"sendTimeOut\":%d}";

int cgiMain() 
{
    FILE* fhtm = NULL;
    char* resp = NULL;
    int respLen = 0;
    JSON_Value* jVaule = NULL;
    JSON_Object* jsonObj = NULL;
    int heartbeatPeriod = -1;
    int reportPeriod = -1;
    int sendTimeOut = -1;

    cgiHeaderContentType("text/html");

    fhtm = fopen("../app.htm", "r");
    if(fhtm == NULL) return -1;

    fread(html_buf, 1, 10240, fhtm);

    fclose(fhtm);

    int sock = attachToHost("127.0.0.1", 8421);

    if(strcmp(cgiRequestMethod, "GET") == 0) {
        sendPacket(sock, requestGetString);
        recvResponse(sock, &resp, &respLen);

        jVaule = json_parse_string(resp);   
        jsonObj = json_object(jVaule);

        if(json_object_has_value(jsonObj, "heartbeatPeriod")) {
            heartbeatPeriod = (int)json_object_get_number(jsonObj, "heartbeatPeriod");
        }
        if(json_object_has_value(jsonObj, "reportPeriod")) {
            reportPeriod = (int)json_object_get_number(jsonObj, "reportPeriod");
        }
        if(json_object_has_value(jsonObj, "sendTimeOut")) {
            sendTimeOut = (int)json_object_get_number(jsonObj, "sendTimeOut");
        }
        
        fprintf(cgiOut, html_buf, heartbeatPeriod, reportPeriod, sendTimeOut);
        json_value_free(jVaule);

    } else if(strcmp(cgiRequestMethod, "POST") == 0) {

        cgiFormInteger("heartbeat", &heartbeatPeriod, -1);
        cgiFormInteger("reportbeat", &reportPeriod, -1);
        cgiFormInteger("timeout", &sendTimeOut, -1);

        char sendBuf[200];
        sprintf(sendBuf, requestSetString, heartbeatPeriod, reportPeriod, sendTimeOut);
        sendPacket(sock, sendBuf);
        recvResponse(sock, &resp, &respLen);
        
        fprintf(cgiOut, html_buf, heartbeatPeriod, reportPeriod, sendTimeOut);
    }
    
    return 0;
}
