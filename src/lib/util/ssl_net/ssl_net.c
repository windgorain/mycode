/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-5-11
* Description: 
* History:     
******************************************************************************/

/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_SSLNET

#include "bs.h"

#include <openssl/ssl.h>
#include <openssl/rand.h>

#include "utl/txt_utl.h"

#define CA_LIST "sslcert/root.pem"
#define HOST	"localhost"
#define RANDOM  "sslcert/random.pem"
#define _SSL_NET_DFT_KEY_FILE "sslcert/server.pem"
#define _SSL_NET_DFT_DH_FILE  "sslcert/dh1024.pem"

#define _SSLNET_BIO_TYPE_SSLTCP		(30|0x0400|0x0100)


typedef struct
{
    UINT ulFd;
    SSL_CTX *pstCtx;
    SSL *pstSsl;

    PF_SSLTCP_INNER_CB_FUNC pfAsynCallBackFunc;
    USER_HANDLE_S stUserHandle;
}_SSL_NET_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
    CHAR *pszSslPolicyName;
    CHAR *pszKeyFile;
    CHAR *pszDHFile;
}_SSL_NET_POLICY_S;

static int _SSLNET_SslTcpWrite(BIO *b, const char *in, int inl);
static int _SSLNET_SslTcpRead(BIO *b, char *out, int outl);
static int _SSLNET_SslTcpPuts(BIO *bp, const char *str);
static int _SSLNET_SslTcpFree(BIO *a);
static long _SSLNET_SslTcpCtrl(BIO *b, int cmd, long num, void *ptr);
static int _SSLNET_SslTcpNew(BIO *bi);



static _SSL_NET_POLICY_S g_stSslNetPolicyDefault = {{0}, "default", _SSL_NET_DFT_KEY_FILE, _SSL_NET_DFT_DH_FILE};
static DLL_HEAD_S g_stSslNetPolicyList = DLL_HEAD_INIT_VALUE(&g_stSslNetPolicyList);
static BIO_METHOD g_stSslNetMethodsSslTcp =
	{
	_SSLNET_BIO_TYPE_SSLTCP,
	"ssltcp",
	_SSLNET_SslTcpWrite,
	_SSLNET_SslTcpRead,
	_SSLNET_SslTcpPuts,
	NULL, /* sock_gets, */
	_SSLNET_SslTcpCtrl,
	_SSLNET_SslTcpNew,
	_SSLNET_SslTcpFree,
	NULL,
	};

static int _SSLNET_SslTcpWrite(BIO *b, const char *in, int inl)
{
    BS_STATUS eRet;
    UINT ulWriteLen;

    eRet = SSLTCP_Write((UINT)(b->num), (UCHAR*)in, (UINT)inl, &ulWriteLen);
    BIO_clear_retry_flags(b);

    if (eRet != BS_OK)
    {
        return -1;
    }
    
    if (ulWriteLen == 0)
   	{
		BIO_set_retry_write(b);
   	}
    
    return ulWriteLen;
}

static int _SSLNET_SslTcpRead(BIO *b, char *out, int outl)
{
    BS_STATUS eRet;
    UINT ulReadSize = 0;

    if (out != NULL)
	{
        eRet = SSLTCP_Read(b->num, out, outl, &ulReadSize);
    	BIO_clear_retry_flags(b);

        if (BS_OK != eRet)
        {
            return -1;
        }

        if (ulReadSize == 0)
        {
            BIO_set_retry_read(b);
        }
	}

    return ulReadSize;
}

static int _SSLNET_SslTcpPuts(BIO *bp, const char *str)
{
    int n,ret;

    n = strlen(str);
    ret=_SSLNET_SslTcpWrite(bp,str,n);
    
    return(ret);
}

static int _SSLNET_SslTcpFree(BIO *a)
{
	if (a == NULL)
    {
        return(0);
	}
    
	if (a->shutdown)
	{
		if (a->init)
		{
			SSLTCP_Close(a->num);
		}

		a->init=0;
		a->flags=0;
	}

	return(1);
}

static long _SSLNET_SslTcpCtrl(BIO *b, int cmd, long num, void *ptr)
{
	long ret=1;
	int *ip;

	switch (cmd)
	{
    	case BIO_CTRL_RESET:
    		num=0;
    	case BIO_C_FILE_SEEK:
    		ret=0;
    		break;
    	case BIO_C_FILE_TELL:
    	case BIO_CTRL_INFO:
    		ret=0;
    		break;
    	case BIO_C_SET_FD:
    		_SSLNET_SslTcpFree(b);
    		b->num= *((int *)ptr);
    		b->shutdown=(int)num;
    		b->init=1;
    		break;
    	case BIO_C_GET_FD:
    		if (b->init)
			{
    			ip=(int *)ptr;
    			if (ip != NULL)
                {
                    *ip=b->num;
    			}
    			ret=b->num;
			}
    		else
    		{
    			ret= -1;
    		}
    		break;
    	case BIO_CTRL_GET_CLOSE:
    		ret=b->shutdown;
    		break;
    	case BIO_CTRL_SET_CLOSE:
    		b->shutdown=(int)num;
    		break;
    	case BIO_CTRL_PENDING:
    	case BIO_CTRL_WPENDING:
    		ret=0;
    		break;
    	case BIO_CTRL_DUP:
    	case BIO_CTRL_FLUSH:
    		ret=1;
    		break;
    	default:
    		ret=0;
    		break;
	}
    
	return(ret);
}

static int _SSLNET_SslTcpNew(BIO *bi)
{
	bi->init=0;
	bi->num=0;
	bi->ptr=NULL;
	bi->flags=0;
	return(1);
}


static BS_STATUS _SSLNET_Init()
{
    static BOOL_T bIsInit = FALSE;
   
    if (bIsInit == FALSE)
    {
        bIsInit = TRUE;
        SSL_library_init();
        SSL_load_error_strings();
    }

    return BS_OK;
}

static int _SSLNET_PasswordCb(OUT CHAR *buf, IN int size, IN int rwflag, IN void *userdata)
{
    int iSize;

    iSize = strlen(userdata);
    
    if(size <= iSize)
    {
        return(0);
    }

    TXT_Strlcpy(buf, userdata, size);

    return iSize;
}

static void _SSLNET_LoadDhParams(IN HANDLE hCtx, IN CHAR *pszFileName)
{
    SSL_CTX *ctx = (SSL_CTX *)hCtx;
    DH *ret=0;
    BIO *bio;

    if ((bio=BIO_new_file(pszFileName, "r")) == NULL)
    {
        return;
    }

    ret=PEM_read_bio_DHparams(bio,NULL,NULL,NULL);

    BIO_free(bio);
    
    if(SSL_CTX_set_tmp_dh(ctx,ret)<0)
    {
        return;
    }
}

static void _SSLNET_GenerateEphRsaKey(IN HANDLE hCtx)
{
    SSL_CTX *ctx = (SSL_CTX *)hCtx;
    RSA *rsa;

    rsa= RSA_BuildKey(2048);
 
    if (!SSL_CTX_set_tmp_rsa(ctx,rsa))
    {
        return;
    }

    RSA_free(rsa);
}

static _SSL_NET_POLICY_S * _SSLNET_GetPolicyByName(IN CHAR *pszSslPolicy)
{
    _SSL_NET_POLICY_S *pstNode;
    
    if (NULL == pszSslPolicy)
    {
        return &g_stSslNetPolicyDefault;
    }

    DLL_SCAN(&g_stSslNetPolicyList, pstNode)
    {
        if (strcmp(pstNode->pszSslPolicyName, pszSslPolicy) == 0)
        {
            return pstNode;
        }
    }

    return NULL;
}

static SSL_CTX * _SSLNET_CreateNewCtx(IN CHAR *pszSslPolicy)
{
    SSL_METHOD *meth;
    SSL_CTX *ctx;
    _SSL_NET_POLICY_S *pstPolicyNode;
    CHAR *pszKeyFile;
    CHAR *pszDhFile;

    pstPolicyNode = _SSLNET_GetPolicyByName(pszSslPolicy);
    pszKeyFile = pstPolicyNode->pszKeyFile;
    pszDhFile = pstPolicyNode->pszDHFile;

    meth=(SSL_METHOD*)SSLv23_method();
    ctx=SSL_CTX_new(meth);

    /* 加载证书*/
    if(!(SSL_CTX_use_certificate_file(ctx, pszKeyFile, SSL_FILETYPE_PEM)))
    {
        return 0;
    }

#if 0
    SSL_CTX_set_default_passwd_cb_userdata(ctx, pszPassWd);
    SSL_CTX_set_default_passwd_cb(ctx, _SSLNET_PasswordCb);
#endif

    if(!(SSL_CTX_use_PrivateKey_file(ctx, pszKeyFile, SSL_FILETYPE_PEM)))
    {
        return 0;
    }

    /* Load the CAs we trust*/
    if(!(SSL_CTX_load_verify_locations(ctx, CA_LIST, 0)))
    {
        return 0;
    }

    SSL_CTX_set_verify_depth(ctx, 1);

    /* Load randomness */
    if(!(RAND_load_file(RANDOM,1024*1024)))
    {
        return 0;
    }

    _SSLNET_LoadDhParams(ctx, pszDhFile);
    _SSLNET_GenerateEphRsaKey(ctx);
       
    return ctx;
}

static VOID _SSLNET_DelCtx(IN SSL_CTX *ctx)
{
    SSL_CTX_free(ctx);
}

static BIO * _SSLNET_New_BioSslTcp(IN UINT ulFd, IN UINT ulCloseFlag)
{
	BIO *pstBio;

	pstBio=BIO_new(&g_stSslNetMethodsSslTcp);
	if (pstBio == NULL)
    {
        return NULL;
	}
    
	BIO_set_fd(pstBio, ulFd, ulCloseFlag);
	return pstBio;
}

BS_STATUS SSLNET_Create(IN UINT ulFamily, IN CHAR *pszSslPolicy, OUT HANDLE *phSslHandle)
{
    _SSL_NET_S *pstSslNet = NULL;
    SSL_CTX *pstCtx = NULL;
    UINT ulFd = 0;

    _SSLNET_Init();

    pstSslNet = MEM_ZMalloc(sizeof(_SSL_NET_S));
    if (NULL == pstSslNet)
    {
        RETURN(BS_NO_MEMORY);
    }

    pstCtx = _SSLNET_CreateNewCtx(pszSslPolicy);
    if (NULL == pstCtx)
    {
        MEM_Free(pstSslNet);
        RETURN(BS_NO_MEMORY);
    }

    if (0 == (ulFd = SSLTCP_Create("tcp", ulFamily, NULL)))
    {
        _SSLNET_DelCtx(pstCtx);
        MEM_Free(pstSslNet);
        RETURN(BS_ERR);
    }

    pstSslNet->pstCtx = pstCtx;
    pstSslNet->ulFd = ulFd;

    *phSslHandle = pstSslNet;

    return BS_OK;    
}

/*ip/port:主机序*/
BS_STATUS SSLNET_Listen(IN HANDLE hSslHandle, UINT ulLocalIp, IN USHORT usPort, IN USHORT ulBackLog)
{
    _SSL_NET_S *pstSslNet = (_SSL_NET_S*)hSslHandle;

    if (pstSslNet == NULL)
    {
        BS_WARNNING(("Null Ptr"));
        RETURN(BS_NULL_PARA);
    }

    if (0 == pstSslNet->ulFd)
    {
        RETURN(BS_NO_SUCH);
    }

    return SSLTCP_Listen(pstSslNet->ulFd, ulLocalIp, usPort, ulBackLog);
}

BS_STATUS SSLNET_Accept(IN HANDLE hListenSslHandle, OUT HANDLE *phAcceptSslHandle)
{
    _SSL_NET_S *pstSslNet = (_SSL_NET_S*)hListenSslHandle;
    _SSL_NET_S *pstNewSslNet = NULL;
    SSL *pstNewSsl;
    UINT ulAcceptFd = 0;
    BS_STATUS eRet;
    BIO *pstBio;

    BS_DBGASSERT(NULL != phAcceptSslHandle);

    *phAcceptSslHandle = 0;

    if (pstSslNet == NULL)
    {
        BS_WARNNING(("Null Ptr"));
        RETURN(BS_NULL_PARA);
    }

    if (0 == pstSslNet->ulFd)
    {
        RETURN(BS_NO_SUCH);
    }

    eRet = SSLTCP_Accept(pstSslNet->ulFd, &ulAcceptFd);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    pstNewSslNet = MEM_ZMalloc(sizeof(_SSL_NET_S));
    if (NULL == pstNewSslNet)
    {
        SSLTCP_Close(ulAcceptFd);
        RETURN(BS_NO_MEMORY);
    }

    pstNewSsl = SSL_new(pstSslNet->pstCtx);
    if (NULL == pstNewSsl)
    {
        SSLTCP_Close(ulAcceptFd);
        MEM_Free(pstNewSslNet);
        RETURN(BS_ERR);
    }

    pstBio = _SSLNET_New_BioSslTcp(ulAcceptFd, BIO_NOCLOSE);
    if (NULL == pstBio)
    {
        SSL_free(pstNewSsl);
        SSLTCP_Close(ulAcceptFd);
        MEM_Free(pstNewSslNet);
        RETURN(BS_ERR);
    }

    SSL_set_bio(pstNewSsl, pstBio, pstBio);

    if (SSL_accept(pstNewSsl) <= 0)
    {
        SSL_free(pstNewSsl);
        SSLTCP_Close(ulAcceptFd);
        MEM_Free(pstNewSslNet);
        RETURN(BS_AGAIN); /* 有可能协商错误 */
    }

    pstNewSslNet->ulFd = ulAcceptFd;
    pstNewSslNet->pstSsl = pstNewSsl;

    *phAcceptSslHandle = pstNewSslNet;

    return BS_OK;    
}

INT SSLNET_Send(IN HANDLE hSslHandle, IN UCHAR *pucBuf, IN UINT ulLen, IN UINT ulFlag)
{
    _SSL_NET_S *pstSslNet = (_SSL_NET_S*)hSslHandle;

    BS_DBGASSERT(0 != hSslHandle);
    
    return SSL_write(pstSslNet->pstSsl, pucBuf, ulLen);
}

BS_STATUS SSLNET_Recv
(
    IN HANDLE hSslHandle,
    OUT UCHAR *pucBuf,
    IN UINT ulLen,
    OUT UINT *puiReadLen,
    IN UINT ulFlag
)
{
    _SSL_NET_S *pstSslNet = (_SSL_NET_S*)hSslHandle;
    INT iReadLen;

    BS_DBGASSERT(0 != hSslHandle);

    *puiReadLen = 0;
    
    iReadLen = SSL_read(pstSslNet->pstSsl, pucBuf, ulLen);
    if (iReadLen < 0)
    {
        return BS_ERR;
    }

    if (iReadLen == 0)
    {
        return BS_PEER_CLOSED;
    }

    *puiReadLen = (UINT)iReadLen;

    return BS_OK;
}

BS_STATUS SSLNET_Close(IN HANDLE hSslHandle)
{
    _SSL_NET_S *pstSslNet = (_SSL_NET_S*)hSslHandle;

    if (pstSslNet != NULL)
    {
        if (pstSslNet->pstSsl != NULL)
        {
            SSL_shutdown(pstSslNet->pstSsl);
            SSL_free(pstSslNet->pstSsl);
        }
        if (pstSslNet->pstCtx != NULL)
        {
            _SSLNET_DelCtx(pstSslNet->pstCtx);
        }
        if (pstSslNet->ulFd != 0)
        {
            SSLTCP_Close(pstSslNet->ulFd);
        }

        MEM_Free(pstSslNet);
    }

    return BS_OK;
}

static BS_STATUS _SSLNET_AsynCallBack(IN UINT hSslTcpId, IN UINT ulEvent, IN USER_HANDLE_S *pstUserHandle)
{
    _SSL_NET_S *pstSslNet = (_SSL_NET_S*)pstUserHandle->ahUserHandle[0];

    return pstSslNet->pfAsynCallBackFunc(pstSslNet, ulEvent, (HANDLE)&(pstSslNet->stUserHandle));
}

BS_STATUS SSLNET_SetAsyn(IN HANDLE hSslHandle, IN PF_SSLTCP_INNER_CB_FUNC pfFunc, IN USER_HANDLE_S *pstUserHandle)
{
    _SSL_NET_S *pstSslNet = (_SSL_NET_S*)hSslHandle;
    USER_HANDLE_S stUserHandle;

    if (NULL == pstSslNet)
    {
        BS_DBGASSERT(0);
        RETURN(BS_NULL_PARA);
    }

    pstSslNet->pfAsynCallBackFunc = pfFunc;
    pstSslNet->stUserHandle = *pstUserHandle;

    stUserHandle.ahUserHandle[0] = hSslHandle;

    return SSLTCP_SetAsyn(NULL, pstSslNet->ulFd, SSLTCP_EVENT_ALL, _SSLNET_AsynCallBack, &stUserHandle);
}

BS_STATUS SSLNET_UnSetAsyn(IN HANDLE hSslHandle)
{
    _SSL_NET_S *pstSslNet = hSslHandle;

    if (NULL == pstSslNet)
    {
        BS_DBGASSERT(0);
        RETURN(BS_NULL_PARA);
    }

    return SSLTCP_UnSetAsyn(pstSslNet->ulFd);
}

/* 返回主机序IP和Port */
BS_STATUS SSLNET_GetHostIpPort(IN HANDLE hSslHandle, OUT UINT *pulIp, OUT USHORT *pusPort)
{
    _SSL_NET_S *pstSslNet = (_SSL_NET_S*)hSslHandle;
    UINT ulIp;
    USHORT usPort;

    if (NULL == pstSslNet)
    {
        BS_DBGASSERT(0);
        RETURN(BS_NULL_PARA);
    }

    ulIp = SSLTCP_GetHostIP(pstSslNet->ulFd);
    usPort = SSLTCP_GetHostPort(pstSslNet->ulFd);

    *pulIp = ulIp;
    *pusPort = usPort;

    return BS_OK;
}

/* 返回主机序IP和Port */
BS_STATUS SSLNET_GetPeerIpPort(IN HANDLE hSslHandle, OUT UINT *pulIp, OUT USHORT *pusPort)
{
    _SSL_NET_S *pstSslNet = (_SSL_NET_S*)hSslHandle;
    UINT ulIp;
    USHORT usPort;

    if (NULL == pstSslNet)
    {
        BS_DBGASSERT(0);
        RETURN(BS_NULL_PARA);
    }

    ulIp = SSLTCP_GetPeerIP(pstSslNet->ulFd);
    usPort = SSLTCP_GetPeerPort(pstSslNet->ulFd);

    *pulIp = ulIp;
    *pusPort = usPort;

    return BS_OK;    
}

