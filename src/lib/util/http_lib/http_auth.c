/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-21
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/md5_utl.h"
#include "utl/atomic_utl.h"
#include "utl/txt_utl.h"
#include "utl/kv_utl.h"
#include "utl/time_utl.h"
#include "utl/base64_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/http_lib.h"
#include "utl/http_auth.h"

#define HTTP_AUTH_BASIC_TITLE_LEN  6             

#define _HTTP_AUTH_DIGEST_REALM_MAX_LEN 127
#define _HTTP_AUTH_DIGEST_NONCE_MAX_LEN 127
#define _HTTP_AUTH_DIGEST_OPAQUE_MAX_LEN 127
#define _HTTP_AUTH_DIGEST_ALGORITHM_MAX_LEN 31
#define _HTTP_AUTH_DIGEST_QOP_MAX_LEN 31
#define _HTTP_AUTH_DIGEST_CNONCE_MAX_LEN 15
#define _WEB_PROXY_AUTH_DIGEST_USER_NAME_MAX_LEN 127
#define _WEB_PROXY_AUTH_DIGEST_PASSWORD_MAX_LEN 127
#define _HTTP_AUTH_NC_LEN 8

typedef struct
{
    CHAR szRealm[_HTTP_AUTH_DIGEST_REALM_MAX_LEN + 1];     
    CHAR szNonce[_HTTP_AUTH_DIGEST_NONCE_MAX_LEN + 1];      
    CHAR szOpaque[_HTTP_AUTH_DIGEST_OPAQUE_MAX_LEN + 1];     
    CHAR szAlgorithm[_HTTP_AUTH_DIGEST_ALGORITHM_MAX_LEN + 1];  
    CHAR szQop[_HTTP_AUTH_DIGEST_QOP_MAX_LEN + 1];        
    CHAR szCNonce[_HTTP_AUTH_DIGEST_CNONCE_MAX_LEN + 1];

    volatile int nc;
    CHAR *pcUserName;
    CHAR *pcPassWord;
}_HTTP_AUTH_DIGEST_S;



typedef struct
{
    UINT uiNc;
    CHAR szResponse[2*MD5_LEN+1];
}HTTP_AUTH_RET_S;


static BOOL_T http_auth_IsQopAuth(IN CHAR *pcQop)
{
    ULONG ulTokenLen = 0;
    ULONG ulOffset = 0;
    ULONG ulLen = 0;
    CHAR *pcTmp;
    CHAR *pcFind;

    if (NULL == pcQop)
    {
        return FALSE;
    }

    ulLen = (ULONG)strlen(pcQop);

    while(ulOffset < ulLen)
    {
        pcTmp = pcQop+ulOffset;
        pcFind = TXT_Strnchr(pcTmp, ',', strlen(pcTmp));
        ulTokenLen = pcFind - pcTmp;
        
        if (NULL == pcFind)
        {
            if (0 == (LONG)stricmp("auth", pcQop+ulOffset))
            {
                return TRUE;
            }
            break;
        }
        else
        {
            
            if (ulTokenLen != (ULONG)strlen("auth"))
            {
                ulOffset += (ulTokenLen + 1);
                continue;
            }

            if (0 == (LONG)strnicmp("auth", pcQop+ulOffset, strlen("auth")))
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
    }

    return FALSE;
}



VOID http_auth_GenerateCnonce(IN CHAR *pcCnonce)
{
    ULONG ulRetTimeInSec = 0;  

    
    ulRetTimeInSec = (ULONG) TM_NowInSec();
    sprintf(pcCnonce, "%lx", ulRetTimeInSec);
}


static BS_STATUS http_auth_CalcAuthCode
(
    IN _HTTP_AUTH_DIGEST_S *pstCtrl,
    IN CHAR *pcMethod,
    IN CHAR *pcUri,
    OUT HTTP_AUTH_RET_S *pstAuthRet
)
{
    UCHAR ucQopFlag = FALSE;
    CHAR *pcPassword;
    MD5_CTX stContext;
    UCHAR szDigest[MD5_LEN];
    CHAR szHexDigestFinal[2*MD5_LEN + 1];
    CHAR szHexDigestA1[2*MD5_LEN + 1];
    CHAR szHexDigestA2[2*MD5_LEN + 1];
    CHAR szNcValue[_HTTP_AUTH_NC_LEN + 1];

    if (NULL == pstCtrl)
    {
        return BS_ERR;
    }

    memset(szNcValue, 0, sizeof(szNcValue));

    if (TRUE == http_auth_IsQopAuth(pstCtrl->szQop))
    {
        ucQopFlag = TRUE;
        pstAuthRet->uiNc = ATOM_INC_FETCH(&pstCtrl->nc);
        (VOID)snprintf(szNcValue, sizeof(szNcValue), "%08x", pstAuthRet->uiNc);
    }
    
    else if (pstCtrl->szQop[0] != '\0')
    {
        return BS_ERR;
    }

    pcPassword = pstCtrl->pcPassWord;
    if (NULL == pcPassword)
    {
        pcPassword = "";
    }

    
    (VOID)Mem_Zero(&stContext, sizeof (MD5_CTX));
    MD5UTL_Init(&stContext);
    Mem_Zero(szDigest, MD5_LEN);
    MD5UTL_Update(&stContext, (UCHAR *)pstCtrl->pcUserName, strlen(pstCtrl->pcUserName));
    MD5UTL_Update(&stContext, (UCHAR *)":", 1);
    MD5UTL_Update(&stContext, (UCHAR *)pstCtrl->szRealm, (ULONG)strlen(pstCtrl->szRealm));
    MD5UTL_Update(&stContext, (UCHAR *)":", 1);
    MD5UTL_Update(&stContext, (UCHAR *)pcPassword, strlen(pcPassword));
    MD5UTL_Final(szDigest, &stContext);
    DH_Data2HexString(szDigest, MD5_LEN, szHexDigestA1);
    if (stricmp(pstCtrl->szAlgorithm, "MD5-sess") == 0)
    {
        MD5UTL_Init(&stContext);
        Mem_Zero(szDigest, MD5_LEN);
        MD5UTL_Update(&stContext, (UCHAR *)szHexDigestA1, 2*MD5_LEN);
        MD5UTL_Update(&stContext, (UCHAR *)":", 1);
        MD5UTL_Update(&stContext, (UCHAR *)pstCtrl->szNonce, (ULONG)strlen(pstCtrl->szNonce));
        MD5UTL_Update(&stContext, (UCHAR *)":", 1);
        MD5UTL_Update(&stContext, (UCHAR *)pstCtrl->szCNonce, (ULONG)strlen(pstCtrl->szCNonce));
        MD5UTL_Final(szDigest, &stContext);
        DH_Data2HexString(szDigest, MD5_LEN, szHexDigestA1);
    }


    
    Mem_Zero(&stContext, sizeof (MD5_CTX));
    Mem_Zero(szDigest, MD5_LEN);
    
    MD5UTL_Init(&stContext);
    MD5UTL_Update(&stContext, (UCHAR *)pcMethod, strlen(pcMethod));
    MD5UTL_Update(&stContext, (UCHAR *)":", 1);
    MD5UTL_Update(&stContext, (UCHAR *)pcUri, (ULONG)strlen(pcUri));
    MD5UTL_Final(szDigest, &stContext);
    DH_Data2HexString(szDigest, MD5_LEN, szHexDigestA2);


    
    (VOID)Mem_Zero(&stContext, sizeof (MD5_CTX));
    (VOID)Mem_Zero(szDigest, MD5_LEN);
    MD5UTL_Init(&stContext);
    MD5UTL_Update(&stContext, (UCHAR *)szHexDigestA1, 2*MD5_LEN);
    MD5UTL_Update(&stContext, (UCHAR *)":", 1);
    
    MD5UTL_Update(&stContext, (UCHAR *)pstCtrl->szNonce, (ULONG)strlen(pstCtrl->szNonce));
    if (TRUE == ucQopFlag)
    {
        MD5UTL_Update(&stContext, (UCHAR *)":", 1);
        MD5UTL_Update(&stContext, (UCHAR *)szNcValue, (ULONG)strlen(szNcValue));
        MD5UTL_Update(&stContext, (UCHAR *)":", 1);
        MD5UTL_Update(&stContext, (UCHAR *)pstCtrl->szCNonce, (ULONG)strlen(pstCtrl->szCNonce));
        MD5UTL_Update(&stContext, (UCHAR *)":", 1);
        MD5UTL_Update(&stContext, (UCHAR *)pstCtrl->szQop, (ULONG)strlen(pstCtrl->szQop));
    }
    MD5UTL_Update(&stContext, (UCHAR *)":", 1);
    MD5UTL_Update(&stContext, (UCHAR *)szHexDigestA2, 2*MD5_LEN);
    MD5UTL_Final(szDigest, &stContext);
    DH_Data2HexString(szDigest, MD5_LEN, szHexDigestFinal);

    (VOID)MEM_Copy(pstAuthRet->szResponse, szHexDigestFinal, 2*MD5_LEN);
    pstAuthRet->szResponse[2*MD5_LEN] = '\0';

    return BS_OK;
}

static VOID http_auth_ClearUser(IN _HTTP_AUTH_DIGEST_S *pstCtrl)
{
    if (NULL != pstCtrl->pcUserName)
    {
        MEM_Free(pstCtrl->pcUserName);
        pstCtrl->pcUserName = NULL;
    }

    if (NULL != pstCtrl->pcPassWord)
    {
        MEM_Free(pstCtrl->pcPassWord);
        pstCtrl->pcPassWord = NULL;
    }
}

BS_STATUS HTTP_AUTH_BasicBuild
(
    IN CHAR *pcUserName,
    IN CHAR *pcPassWord,
    OUT CHAR szResult[HTTP_AUTH_DIGEST_LEN + 1]
)
{
    ULONG ulHeaderLen = 0;
    ULONG ulCurrLen = 0;
    CHAR szTmp[HTTP_AUTH_DIGEST_LEN + 1];
    UINT uiUserNameLen;
    UINT uiPassWordLen;
    UINT uiClearAuthCodeLen;

    
    if ((NULL == pcUserName) || (NULL == pcPassWord))
    {
        return BS_BAD_PARA;
    }

    uiUserNameLen = strlen(pcUserName);
    uiPassWordLen = strlen(pcPassWord);
    uiClearAuthCodeLen = uiUserNameLen + uiPassWordLen + 1;

    
    ulHeaderLen = HTTP_AUTH_BASIC_TITLE_LEN;
    ulHeaderLen += uiClearAuthCodeLen * 4/3 + 1; 
    ulHeaderLen ++;  

    if (ulHeaderLen > HTTP_AUTH_DIGEST_LEN)
    {
        BS_DBGASSERT(0);
        return BS_OUT_OF_RANGE;
    }

    (VOID)snprintf(szTmp, HTTP_AUTH_DIGEST_LEN, "%s:%s", pcUserName, pcPassWord);

    
    ulCurrLen += (ULONG)snprintf(szResult + ulCurrLen, ulHeaderLen - ulCurrLen, "Basic ");
    BASE64_Encode((UCHAR*)szTmp, uiClearAuthCodeLen, szResult + ulCurrLen);

    return BS_OK;
}

BOOL_T HTTP_AUTH_IsDigestUnauthorized(IN CHAR *pcWwwAuthenticate)
{
    if (strnicmp(pcWwwAuthenticate, "Digest ", 7) != 0)
    {
        return FALSE;
    }

    return TRUE;
}

HTTP_AUTH_HANDLE HTTP_Auth_ClientCreate()
{
    _HTTP_AUTH_DIGEST_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(_HTTP_AUTH_DIGEST_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    return pstCtrl;
}


BS_STATUS HTTP_Auth_ClientSetAuthContext
(
    IN HTTP_AUTH_HANDLE hAuthHandle,
    IN CHAR *pcWwwAuthenticate 
)
{
    _HTTP_AUTH_DIGEST_S *pstCtrl = hAuthHandle;
    CHAR *pcAuthInfo = pcWwwAuthenticate;
    KV_HANDLE hKv;
    LSTR_S stLstr;
    CHAR *pcRealm;
    CHAR *pcNonce;
    CHAR *pcOpaque;
    CHAR *pcAlgorithm;
    CHAR *pcQop;
    ULONG ulRetTimeInSec = 0;

    if ((NULL == pstCtrl) || (NULL == pcWwwAuthenticate))
    {
        return BS_NULL_PARA;
    }

    if (TRUE != HTTP_AUTH_IsDigestUnauthorized(pcAuthInfo))
    {
        return BS_NOT_SUPPORT;
    }

    pcAuthInfo += 6;   

    hKv = KV_Create(NULL);
    if (NULL == hKv)
    {
        return BS_NO_MEMORY;
    }

    stLstr.pcData = pcAuthInfo;
    stLstr.uiLen = strlen(pcAuthInfo);

    if (BS_OK != KV_Parse(hKv, &stLstr, ',', '='))
    {
        KV_Destory(hKv);
        return BS_ERR;
    }

    http_auth_ClearUser(pstCtrl);
    memset(pstCtrl, 0, sizeof(_HTTP_AUTH_DIGEST_S));

    pcRealm = KV_GetKeyValue(hKv, "realm");
    pcRealm = TXT_StrimString(pcRealm, "\"");
    pcNonce = KV_GetKeyValue(hKv, "nonce");
    pcNonce = TXT_StrimString(pcNonce, "\"");
    pcOpaque = KV_GetKeyValue(hKv, "opaque");
    pcOpaque = TXT_StrimString(pcOpaque, "\"");
    pcAlgorithm = KV_GetKeyValue(hKv, "algorithm");
    pcQop = KV_GetKeyValue(hKv, "qop");
    pcQop = TXT_StrimString(pcQop, "\"");

    
    ulRetTimeInSec = (ULONG) TM_NowInSec();
    sprintf(pstCtrl->szCNonce, "%08lx", ulRetTimeInSec);

    if (NULL != pcRealm)
    {
        strlcpy(pstCtrl->szRealm, pcRealm, sizeof(pstCtrl->szRealm));
    }

    if (NULL != pcNonce)
    {
        strlcpy(pstCtrl->szNonce, pcNonce, sizeof(pstCtrl->szNonce));
    }

    if (NULL != pcOpaque)
    {
        strlcpy(pstCtrl->szOpaque, pcOpaque, sizeof(pstCtrl->szOpaque));
    }

    if (NULL != pcAlgorithm)
    {
        strlcpy(pstCtrl->szAlgorithm, pcAlgorithm, sizeof(pstCtrl->szAlgorithm));
    }

    if (NULL != pcQop)
    {
        strlcpy(pstCtrl->szQop, pcQop, sizeof(pstCtrl->szQop));
    }

    KV_Destory(hKv);

    return BS_OK;
}

VOID HTTP_Auth_ClientDestroy(IN HTTP_AUTH_HANDLE hAuthHandle)
{
    _HTTP_AUTH_DIGEST_S *pstCtrl = hAuthHandle;

    if (NULL == pstCtrl)
    {
        return;
    }

    http_auth_ClearUser(pstCtrl);

    MEM_Free(pstCtrl);
}

BS_STATUS HTTP_Auth_ClientSetUser(IN HTTP_AUTH_HANDLE hAuthHandle, IN CHAR *pcUser, IN CHAR *pcPassword)
{
    _HTTP_AUTH_DIGEST_S *pstCtrl = hAuthHandle;

    if (NULL == pstCtrl)
    {
        return BS_BAD_PARA;
    }

    http_auth_ClearUser(pstCtrl);

    if (NULL != pcUser)
    {
        pstCtrl->pcUserName = TXT_Strdup(pcUser);
        if (NULL == pstCtrl->pcUserName)
        {
            return BS_NO_MEMORY;
        }
    }

    if (NULL != pcPassword)
    {
        pstCtrl->pcPassWord = TXT_Strdup(pcPassword);
        if (NULL == pstCtrl->pcPassWord)
        {
            return BS_NO_MEMORY;
        }
    }

    return BS_OK;
}

BS_STATUS HTTP_AUTH_ClientDigestBuild
(
    IN HTTP_AUTH_HANDLE hAuthHandle,
    IN CHAR *pcMethod,
    IN CHAR *pcUri,
    OUT CHAR szDigest[HTTP_AUTH_DIGEST_LEN + 1]
)
{
    _HTTP_AUTH_DIGEST_S *pstCtrl = hAuthHandle;
    ULONG ulHeaderLen = 0;
    ULONG ulCurrStrLen = 0;
    CHAR *pcTmp = NULL;
    HTTP_AUTH_RET_S stAuthRet;

    
    if ((NULL == pstCtrl) || (NULL == pcMethod) || (NULL == pcUri))
    {
        return BS_NULL_PARA;
    }

    if ((pstCtrl->szRealm[0] == '\0')
        || (pstCtrl->szNonce[0] == '\0')
        || (pstCtrl->pcUserName == NULL))
    {
        return BS_ERR;
    }

    if (BS_OK != http_auth_CalcAuthCode(pstCtrl, pcMethod, pcUri, &stAuthRet))
    {
        return BS_ERR;
    }

    pcTmp = szDigest;
    ulHeaderLen = HTTP_AUTH_DIGEST_LEN + 1;

    
    ulCurrStrLen += (ULONG)snprintf(pcTmp + ulCurrStrLen, ulHeaderLen - ulCurrStrLen, "%s", "Digest ");
    ulCurrStrLen += (ULONG)snprintf(pcTmp + ulCurrStrLen, ulHeaderLen - ulCurrStrLen,
                          "username=\"%s\",realm=\"%s\",nonce=\"%s\",uri=\"%s\"",
                          pstCtrl->pcUserName, pstCtrl->szRealm, pstCtrl->szNonce, pcUri);

    
    if (pstCtrl->szAlgorithm[0] != '\0')
    {
        ulCurrStrLen += (ULONG)snprintf(pcTmp + ulCurrStrLen,
            ulHeaderLen - ulCurrStrLen, ",algorithm=%s", pstCtrl->szAlgorithm);
    }

    
    if (pstCtrl->szQop[0] != '\0')
    {
        ulCurrStrLen += (ULONG)snprintf(pcTmp + ulCurrStrLen, ulHeaderLen - ulCurrStrLen, 
                                        ",qop=\"%s\"", pstCtrl->szQop);
        ulCurrStrLen +=(ULONG)snprintf(pcTmp + ulCurrStrLen, ulHeaderLen - ulCurrStrLen, 
                                          ",nc=%08x", stAuthRet.uiNc);
        ulCurrStrLen += (ULONG)snprintf(pcTmp + ulCurrStrLen, ulHeaderLen - ulCurrStrLen,
                                          ",cnonce=\"%s\"", pstCtrl->szCNonce);
    }

    
    ulCurrStrLen += (ULONG)snprintf(pcTmp + ulCurrStrLen, ulHeaderLen - ulCurrStrLen, 
                                        ",response=\"%s\"", stAuthRet.szResponse);

    if (pstCtrl->szOpaque[0] != '\0')
    {
        ulCurrStrLen += (ULONG)snprintf(pcTmp + ulCurrStrLen, ulHeaderLen - ulCurrStrLen, 
                                        ",opaque=\"%s\"", pstCtrl->szOpaque);
    }

    return BS_OK;
}


