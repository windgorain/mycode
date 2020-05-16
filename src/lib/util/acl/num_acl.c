/*================================================================
*   Created by LiXingang
*   Description: 针对num的acl,比如port、protocol等
*
================================================================*/
#include "bs.h"

#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/acl_string.h"
#include "utl/num_acl.h"

static void numacl_ProcessLine(NUM_ACL_S *num_acl, char *line)
{
    ACL_STR_S acl_str;
    int action;
    UINT num;

    line = TXT_Strim(line);

    if ((*line == '\0') || (*line == '#')) {
        return;
    }

    if (0 != ACLSTR_Simple_Parse(line, &acl_str)) {
        return;
    }

    num = TXT_Str2Ui(acl_str.pattern);
    if (num >= num_acl->size) {
        return;
    }

    action = NUM_ACL_DENY;
    if (acl_str.action[0] == 'b') {
        action = NUM_ACL_BYPASS;
    } else if (acl_str.action[0] == 'p') {
        action = NUM_ACL_PERMIT;
    }

    num_acl->data[num] = (UCHAR)action;

    return;
}

void NumAcl_Init(NUM_ACL_S *num_acl, UCHAR *data, UINT size)
{
    num_acl->size = size;
    num_acl->data = data;
    memset(data, 0, size);
}

int NumAcl_ParseFile(NUM_ACL_S *num_acl, char *config_file)
{
    FILE *fp;
    char buf[256];

    fp = FILE_Open(config_file, FALSE, "rb");
    if (NULL == fp) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    memset(num_acl->data, 0, num_acl->size);

    while(NULL != fgets(buf, sizeof(buf), fp)) {
        numacl_ProcessLine(num_acl, buf);
    }

    fclose(fp);

    return 0;
}

int NumAcl_Match(NUM_ACL_S *num_acl, UINT num)
{
    if (num >= num_acl->size) {
        return NUM_ACL_UNDEF;
    }

    return num_acl->data[num];
}

