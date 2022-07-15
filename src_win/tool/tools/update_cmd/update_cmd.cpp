// update_cmd.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "bs.h"

#include "utl/cff_utl.h"
#include "utl/update_utl.h"

static VOID update_cmd_UpdateCallBack(IN UPDATE_EVENT_E eEvent, IN VOID *pData, IN VOID *pUserHandle)
{
    switch (eEvent)
    {
        case UPDATE_EVENT_START:
        {
            printf("Update start.\r\n");
            break;
        }

        case UPDATE_EVENT_UPDATE_FILE:
        {
			UPDATE_FILE_S *pstUpdFile = (UPDATE_FILE_S*)pData;

            printf("Updating (%d/%d) %s...",
                pstUpdFile->uiCurrentUpdCount,
                pstUpdFile->uiTotleUpdCount,
                pstUpdFile->pcUpdFile);
            break;
        }

		case UPDATE_EVENT_UPDATE_FILE_RESULT:
        {
            UPDATE_FILE_RESULT_E eResult = (UPDATE_FILE_RESULT_E)HANDLE_UINT(pData);
            CHAR *pcResult;

            switch (eResult)
            {
                case UPDATE_FILE_RESULT_OK:
                case UPDATE_FILE_RESULT_IS_NEWEST:
                {
                    pcResult = "OK";
                    break;
                }

                default:
                {
                    pcResult = "Failed";
                    break;
                }
            }

            printf("%s.\r\n", pcResult);
            break;
        }

        case UPDATE_EVENT_END:
        {
            printf("Update finish.\r\n");
            break;
        }
    }
}

static VOID update_cmd_Start(IN CHAR *pcVerPath)
{
    UPD_Update(pcVerPath, NULL, update_cmd_UpdateCallBack, NULL);
}

static VOID update_init()
{
	CFF_HANDLE hCff;
	CHAR *pcVerUrl;
	
	hCff = CFF_INI_Open("update.ini", CFF_FLAG_READ_ONLY);
	if (NULL == hCff)
	{
		return;
	}

	if (BS_OK != CFF_GetPropAsString(hCff, "update", "url", &pcVerUrl))
	{
		CFF_Close(hCff);
		return;
	}

	if ((pcVerUrl != NULL) && (pcVerUrl[0] != '\0'))
	{
		update_cmd_Start(pcVerUrl);
	}

	CFF_Close(hCff);
}

int _tmain(int argc, _TCHAR* argv[])
{
	update_init();
    getch();
	return 0;
}

