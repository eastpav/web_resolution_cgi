#include <stdio.h>
#include <string.h>
#include "cgic.h"
#include "ipc.h"

char html_buf[10240];
#define BUFFERLEN (8192)

int cgiMain() 
{
    FILE* fhtm = NULL;
    cgiHeaderContentType("text/html");

    fhtm = fopen("../upload.htm", "r");
    if(fhtm == NULL) return -1;

    fread(html_buf, 1, 10240, fhtm);

    fclose(fhtm);

    if(strcmp(cgiRequestMethod, "GET") == 0) {
        fprintf(cgiOut, html_buf, "ver1.2.3");
    } else if(strcmp(cgiRequestMethod, "POST") == 0) {
        int size;
        int got;
        cgiFilePtr file;
        FILE* fp = NULL;
        char name[1024];
        char buffer[BUFFERLEN];

        if(cgiFormFileName("file", name, sizeof(name)) != cgiFormSuccess) {
            fprintf(stderr,"could not retrieve filename\n");
            return -1;
        }

        cgiFormFileSize("file", &size);

        if(cgiFormFileOpen("file", &file) != cgiFormSuccess) {
            fprintf(stderr,"could not open file\n");
            return -1;
        }
    
        fp = fopen("./update.tar.gz", "wb");
        if(fp == NULL) {
            fprintf(stderr,"could create file\n");
            return -1;
        }

        while(cgiFormFileRead(file, buffer, BUFFERLEN, &got) == cgiFormSuccess) {
            if(got > 0) {
                fwrite(buffer, 1, got, fp);
            }
        }

        cgiFormFileClose(file);
        fclose(fp);
    
        fprintf(cgiOut, html_buf, "ver1.2.4");
    }
    
    return 0;    
}
