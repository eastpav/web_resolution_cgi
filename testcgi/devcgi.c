#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "cgic.h"
#include "parson.h"
#include "ipc.h"

char html_buf[10240];

char requestGetString[] = "{\"cmd\":\"DEV_CONF\", \"method\":\"GET\"}";
char requestSetString[] = "{\"cmd\":\"DEV_CONF\", \"method\":\"SET\",\"devPeriod\":%d,\"recvPeriod\":%d,\"freq\":%d}";

int cgiMain() 
{
    FILE* fhtm = NULL;
    char* resp = NULL;
    int respLen = 0;
    JSON_Value* jVaule = NULL;
    JSON_Object* jsonObj = NULL;
    int devPeriod = -1;
    int recvPeriod = -1;
    int freq = -1;

    cgiHeaderContentType("text/html");

    fhtm = fopen("../dev.htm", "r");
    if(fhtm == NULL) return -1;

    fread(html_buf, 1, 10240, fhtm);

    fclose(fhtm);
    int sock = attachToHost("127.0.0.1", 8421);

    if(strcmp(cgiRequestMethod, "GET") == 0) {
        sendPacket(sock, requestGetString);
        recvResponse(sock, &resp, &respLen);

        jVaule = json_parse_string(resp);   
        jsonObj = json_object(jVaule);

        if(json_object_has_value(jsonObj, "devPeriod")) {
            devPeriod = (int)json_object_get_number(jsonObj, "devPeriod");
        }
        if(json_object_has_value(jsonObj, "recvPeriod")) {
            recvPeriod = (int)json_object_get_number(jsonObj, "recvPeriod");
        }
        if(json_object_has_value(jsonObj, "freq")) {
            freq = (int)json_object_get_number(jsonObj, "freq");
        }
        
        fprintf(cgiOut, html_buf, devPeriod, recvPeriod, freq);
        json_value_free(jVaule);

    } else if(strcmp(cgiRequestMethod, "POST") == 0) {
        cgiFormInteger("beacon", &devPeriod, 20);
        cgiFormInteger("receive", &recvPeriod, -1);
        cgiFormInteger("freq", &freq, -1);

        char sendBuf[200];
        sprintf(sendBuf, requestSetString, devPeriod, recvPeriod, freq);
        sendPacket(sock, sendBuf);
        recvResponse(sock, &resp, &respLen);

        fprintf(cgiOut, html_buf, devPeriod, recvPeriod, freq);
    }
    
    return 0;
}
