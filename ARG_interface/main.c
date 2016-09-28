#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "reactor.h"
#include "parson.h"

char badResponse[] = "{\"cmd\":\"unknown\"}";

char* processBaseInfo(JSON_Object* jsonObj)
{
    return "{\"cmd\":\"BASE_INFO\", \"method\":\"response\",\"devId\":\"EEEEEEE000000\"}";
}

char* processNetConf(JSON_Object* jsonObj)
{
    char* method = NULL;
    method = (char*)json_object_get_string(jsonObj, "method");
    if(strcmp(method, "GET") == 0 || strcmp(method, "get") == 0) {
        return "{\"cmd\":\"NET_CONF\", \"method\":\"response\",\"serverIp\":\"112.124.96.78\",\"serverPort\":5600,\"interval\":120}";
    } else if(strcmp(method, "SET") == 0 || strcmp(method, "set") == 0) {
        return "{\"cmd\":\"NET_CONF\", \"method\":\"response\",\"serverIp\":\"112.124.96.78\",\"serverPort\":5600,\"interval\":120}";
    }
    
    return NULL;
}

char* processAppConf(JSON_Object* jsonObj)
{
    char* method = NULL;
    method = (char*)json_object_get_string(jsonObj, "method");
    if(strcmp(method, "GET") == 0 || strcmp(method, "get") == 0) {
        return "{\"cmd\":\"APP_CONF\", \"method\":\"response\",\"heartbeatPeriod\":125,\"reportPeriod\":3600,\"sendTimeOut\":120}";
    } else if(strcmp(method, "SET") == 0 || strcmp(method, "set") == 0) {
        return "{\"cmd\":\"APP_CONF\", \"method\":\"response\",\"heartbeatPeriod\":125,\"reportPeriod\":3600,\"sendTimeOut\":120}";
    }

    return NULL;
}

char* processDevConf(JSON_Object* jsonObj)
{
    char* method = NULL;
    method = (char*)json_object_get_string(jsonObj, "method");
    if(strcmp(method, "GET") == 0 || strcmp(method, "get") == 0) {
        return "{\"cmd\":\"DEV_CONF\", \"method\":\"response\",\"devPeriod\":700,\"recvPeriod\":1,\"freq\":120}";
    } else if(strcmp(method, "SET") == 0 || strcmp(method, "set") == 0) {
        return "{\"cmd\":\"DEV_CONF\", \"method\":\"response\",\"devPeriod\":700,\"recvPeriod\":1,\"freq\":120}";
    }

    return NULL;
}

void dataProcessing(int sock, char* dataReceived, int receivedLen)
{
    comHead_t head;
    JSON_Value* jVaule = NULL;
    JSON_Object* jsonObj = NULL;
    char* cmd = NULL;
    char* response = NULL;
    
    jVaule = json_parse_string(dataReceived);   
    jsonObj = json_object(jVaule);

    cmd = (char*)json_object_get_string(jsonObj, "cmd");
    if(cmd != NULL) {
        /* 命令分派 */
        if(strcmp(cmd, "BASE_INFO") == 0 || strcmp(cmd, "base_info") == 0) {
            response = processBaseInfo(jsonObj);
        } else if(strcmp(cmd, "NET_CONF") == 0 || strcmp(cmd, "net_conf") == 0) {
            response = processNetConf(jsonObj);
        } else if(strcmp(cmd, "APP_CONF") == 0 || strcmp(cmd, "app_conf") == 0) {
            response = processAppConf(jsonObj);
        } else if(strcmp(cmd, "DEV_CONF") == 0 || strcmp(cmd, "dev_conf") == 0) {
            response = processDevConf(jsonObj);
        }
    } else {
        response = badResponse;
    }

    json_value_free(jVaule);

    head.headMagic = HEAD_MAGIC;
    head.dataLen = strlen(response);
    send(sock, (char*)&head, sizeof(head), 0);
    send(sock, response, head.dataLen, 0);

    //if(response != badResponse) free(response);
}


int main(void)
{
    setProcessHandle(dataProcessing);

    startReactor(TRUE, 8421);

    return 0;
}
