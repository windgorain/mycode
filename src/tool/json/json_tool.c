/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/cjson.h"
#include "utl/file_utl.h"

static void help()
{
    printf("Usage: \r\n");
    printf("json_tool filename \r\n");
}

static BS_STATUS _parseline(char *line, int id)
{
    cJSON *json;

    json = cJSON_Parse(line);
    if (! json) {
        RETURN(BS_PARSE_FAILED);
    }

    cJSON_DeleteItemFromObject(json, "source");
    cJSON_AddNumberToObject(json, "id", id);

    printf("%s\n", cJSON_PrintUnformatted(json));

    cJSON_Delete(json);

    return 0;
}

static BS_STATUS _ParseFingerFile(char* file)
{
    FILE *fp;
    char line[8192*2];
    int len;
    BS_STATUS ret=BS_OK;
    int id = 1;

    fp = fopen(file, "rb");
    if (NULL == fp) {
        return BS_CAN_NOT_OPEN;
    }

    do {
        len = FILE_ReadLine(fp, line, sizeof(line), '\n');
        if ((len < sizeof(line)) && (len > 0)) {
            //printf("len=%d\r\n", len);
            ret = _parseline(line, id);
            id ++;
        }
    } while((len > 0) && (ret == BS_OK));

    return ret;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        help();
        return -1;
    }

    _ParseFingerFile(argv[1]);

    return 0;
}

