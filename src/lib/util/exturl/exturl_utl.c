
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/ctype_utl.h"
#include "utl/exturl_utl.h"

/* 
 扩展的URL地址解析.
   通常的地址是 http:
   扩展一下可以为: yyy:
*/


BS_STATUS EXTURL_Parse
(
	IN CHAR *pcExtUrl,
	OUT EXTURL_S *pstExtUrl
)
{
	CHAR *pcProtocol;
	CHAR *pcAddress;
	CHAR *pcPath;
	CHAR *pcPort = NULL;
	UINT uiPort = 0;
	CHAR *pcSplit;
	UINT uiLen;
	CHAR szPort[6];
	UINT uiPortStrlen;
	
	
	pcProtocol = pcExtUrl;
	pcSplit = strstr(pcProtocol, "://");
	if (NULL == pcSplit)
	{
		return BS_ERR;
	}

	uiLen = pcSplit - pcProtocol;
	if (uiLen > EXTURL_MAX_PROTOCOL_LEN)
	{
		return BS_OUT_OF_RANGE;
	}

	TXT_Strlcpy(pstExtUrl->szProtocol, pcProtocol, uiLen + 1);

	
	pcSplit += 3;
	pcAddress = pcSplit;
	pcPath = strchr(pcAddress, '/');
	if (NULL != pcPath)
	{
		if (strlen(pcPath) > EXTURL_MAX_PATH_LEN)
		{
			return BS_OUT_OF_RANGE;
		}

		TXT_Strlcpy(pstExtUrl->szPath, pcPath, sizeof(pstExtUrl->szPath));

		uiLen = pcPath - pcAddress;	
	}
	else
	{
		pstExtUrl->szPath[0] = '/';
		pstExtUrl->szPath[1] = '\0';
		
		uiLen = strlen(pcAddress);	
	}

	
	pcSplit = TXT_Strnchr(pcAddress, ':', uiLen);
	if (pcSplit != NULL)
	{
		
		pcPort = pcSplit + 1;
		uiPortStrlen = uiLen - (pcPort - pcAddress);
		if (uiPortStrlen >= sizeof(szPort))
		{
			return BS_OUT_OF_RANGE;
		}
		TXT_Strlcpy(szPort, pcPort, uiPortStrlen + 1);
        uiPort = strtoul(pcPort, NULL, 10);

		uiLen = pcSplit - pcAddress; 
	}

	
	if (uiLen > EXTURL_MAX_ADDRESS_LEN)
	{
		return BS_OUT_OF_RANGE;
	}

	TXT_Strlcpy(pstExtUrl->szAddress, pcAddress, uiLen + 1);

	pstExtUrl->usPort = uiPort;

	return BS_OK;
}


