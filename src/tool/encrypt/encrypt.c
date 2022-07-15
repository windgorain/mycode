/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-18
* Description: 
* History:     
******************************************************************************/

#include <stdio.h>
#include <string.h>

/* encrypt source-filename password dest-filename */
int main(int argc, char **argv)
{
    FILE *fp, *fq;
    unsigned char ucChar;
    char *pcPassword;
    int iPassWordLen;
    int iPassWordIndex = 0;

    if (argc < 4)
    {
        printf("%s source-filename password dest-filename\r\n", argv[0]);
        return -1;
    }

    pcPassword = argv[2];
    iPassWordLen = strlen(pcPassword);

    fp = fopen(argv[1], "rb+");
    if (fp == NULL)
    {
        printf("Can't open file %s\r\n", argv[1]);
        return -1;
    }
    
    fq = fopen(argv[3], "wb+");
    if (fq == NULL)
    {
        printf("Can't open file %s\r\n", argv[3]);
        fclose(fp);
        return -1;
    }

    ucChar = (unsigned char)fgetc(fp);
    while(! feof(fp))
    {
        ucChar = ucChar ^ pcPassword[iPassWordIndex];
        iPassWordIndex ++;
        if (iPassWordIndex >= iPassWordLen)
        {
            iPassWordIndex = 0;
        }

        fputc(ucChar, fq);

        ucChar = (unsigned char)fgetc(fp);
    }

    fclose(fp);
    fclose(fq);

    return 0;
}

