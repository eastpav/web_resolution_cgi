#include <stdio.h>
#include "cgic.h"

char html_buf[10240];

int cgiMain() 
{
    FILE* fhtm = NULL;
    cgiHeaderContentType("text/html");

    fhtm = fopen("../test.htm", "r");
    if(fhtm == NULL) return -1;

    fread(html_buf, 1, 10240, fhtm);

    fclose(fhtm);

    fprintf(cgiOut, html_buf, "fengzi");
    
    return 0;    
}
