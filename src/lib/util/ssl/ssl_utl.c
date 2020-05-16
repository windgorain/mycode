/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-5
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "openssl/ssl.h"
#include "openssl/err.h"
#include "utl/socket_utl.h"
#include "utl/ssl_utl.h"

static void * g_ssl_utl_dft_ctx = NULL;

static void *sslutl_Ctx_CreateDft()
{
    if (g_ssl_utl_dft_ctx != NULL) {
        return g_ssl_utl_dft_ctx;
    }

    g_ssl_utl_dft_ctx = SSL_UTL_Ctx_Create(0, 0);

    return g_ssl_utl_dft_ctx;
}

VOID SSL_UTL_Init()
{
    static BOOL_T bInited = FALSE;

    if (bInited == TRUE) {
        return;
    }

    bInited = TRUE;
    
    SSLeay_add_ssl_algorithms();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
}

VOID * SSL_UTL_Ctx_Create(int min_version, int max_version)
{
    SSL_CTX *pstCtx;
    void *pstMethod;

    if (min_version == 0) {
        min_version = SSL3_VERSION;
    }

    if (max_version == 0) {
        max_version = TLS_MAX_VERSION;
    }

    SSL_UTL_Init();

    pstMethod = (void*)SSLv23_method();
    if (NULL == pstMethod) {
        return NULL;
    }

    pstCtx = SSL_CTX_new (pstMethod);
    if (NULL == pstCtx) {
        return NULL;
    }

#ifdef SSL_CTX_set_max_proto_version
    if (SSL_CTX_set_min_proto_version(pstCtx, min_version) == 0) {
        SSL_UTL_Ctx_Free(pstCtx);
        return NULL;
    }
    if (SSL_CTX_set_max_proto_version(pstCtx, max_version) == 0) {
        SSL_UTL_Ctx_Free(pstCtx);
        return NULL;
    }
#endif

    SSL_CTX_set_options(pstCtx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

    return pstCtx;
}

VOID SSL_UTL_Ctx_Free(IN VOID *pstSslCtx)
{
    SSL_CTX_free(pstSslCtx);
}

VOID SSL_UTL_Ctx_DisableSsl3(IN VOID *pstSslCtx, IN BOOL_T bDisable)
{
	if (bDisable == TRUE)
	{
		SSL_CTX_set_options(pstSslCtx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
	}
	else
	{
		SSL_CTX_set_options(pstSslCtx, SSL_OP_NO_SSLv2);
	}
}

BS_STATUS SSL_UTL_Ctx_LoadCert(IN VOID *pstSslCtx, IN CHAR *pcCACert, IN CHAR *pcLocalCert, IN CHAR *pcKeyFile)
{
    SSL_CTX *pstCtx = pstSslCtx;

    if (!SSL_CTX_load_verify_locations(pstCtx, pcCACert, NULL))
    {
        return BS_ERR;
    }
 
    if (SSL_CTX_use_certificate_file(pstCtx, pcLocalCert, SSL_FILETYPE_PEM) <= 0)
    {
        return BS_ERR;
    }
 
    if (SSL_CTX_use_PrivateKey_file(pstCtx, pcKeyFile, SSL_FILETYPE_PEM) <= 0)
    {
        return BS_ERR;
    }

    // 判定私钥是否正确
    if (!SSL_CTX_check_private_key(pstCtx))
    {
        return BS_ERR;
    }
 
    return BS_OK;
}

BS_STATUS SSL_UTL_Ctx_LoadSelfSignCert(IN VOID *pstSslCtx, IN PKI_DOMAIN_CONFIGURE_S *pstConf/* 可以为NULL */)
{
    SSL_CTX *pstCtx = pstSslCtx;
    X509 *pstX509Cert;
    EVP_PKEY *pstEvpPkey;

    if (0 != PKI_GetSelfSignCertAndKey(pstConf, &pstX509Cert, &pstEvpPkey))
    {
        return BS_ERR;
    }

    SSL_CTX_use_certificate(pstCtx, pstX509Cert);
    SSL_CTX_use_PrivateKey(pstCtx, pstEvpPkey);

    X509_free(pstX509Cert);
    EVP_PKEY_free(pstEvpPkey);

    return BS_OK;
}

VOID SSL_UTL_Ctx_SetVerifyPeer(IN VOID *pstSslCtx, IN BOOL_T bVerifyPeer)
{
    SSL_CTX *pstCtx = pstSslCtx;

    // 要求校验对方证书  
    if (bVerifyPeer == TRUE)
    {
        SSL_CTX_set_verify(pstCtx, SSL_VERIFY_PEER, NULL);
    }
    else
    {
        SSL_CTX_set_verify(pstCtx, SSL_VERIFY_NONE, NULL);
    }
}

int SSL_UTL_Ctx_LoadVerifyLocations(IN VOID *pstSslCtx, IN CHAR *caFile, IN CHAR *caPath)
{
    if(!SSL_CTX_load_verify_locations(pstSslCtx, caFile, caPath))  { 
        return -1;
    }

    return 0;
}

void * SSL_UTL_BlockConnect(unsigned int ip/*netorder*/, unsigned short port/*netorder*/, char * host_name, int timeout/*us, 0:not set*/)
{  
    int server_fd;
    SSL_CTX *ssl_ctx;
    SSL *ssl_handle;
    struct sockaddr_in server_sockaddr;  
    int err;
    struct timeval timeo;

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  
    if (server_fd < 0)  {  
        return NULL;
    }

    if (timeout > 0) {
        timeo.tv_sec = timeout/1000000;
        timeo.tv_usec = timeout % 1000000;
        setsockopt(server_fd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
    }

    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = port;
    server_sockaddr.sin_addr.s_addr = ip;

    err = connect(server_fd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr));  
    if (err < 0)  {  
        close(server_fd);
        return NULL;
    }  

    ssl_ctx = SSL_UTL_Ctx_Create(0, 0);  
    if (ssl_ctx == NULL)  {
        close(server_fd);
        return NULL;  
    }  

    ssl_handle = SSL_new(ssl_ctx);  
    if (ssl_handle == 0)  {
        SSL_CTX_free(ssl_ctx);
        close(server_fd);
        return NULL;  
    }  

    SSL_set_fd(ssl_handle, server_fd);  
    if (NULL != host_name) {
        SSL_set_tlsext_host_name(ssl_handle, host_name);
    }

    err = SSL_connect(ssl_handle);  
    if (err == -1)  {
        SSL_free(ssl_handle);
        SSL_CTX_free(ssl_ctx);
        close(server_fd);
        return NULL;
    }

    SSL_CTX_free(ssl_ctx);

    return ssl_handle;
} 

VOID * SSL_UTL_New(IN VOID *pstSslCtx)
{
    if (NULL == pstSslCtx) {
        pstSslCtx = sslutl_Ctx_CreateDft();
        if (pstSslCtx == NULL) {
            return NULL;
        }
    }

    return SSL_new(pstSslCtx);
}

VOID SSL_UTL_Free(IN VOID *pstSsl)
{
    if (NULL != pstSsl)
    {
        SSL_free(pstSsl);
    }
}

INT SSL_UTL_SetFd(IN VOID * pstSsl, IN INT iFd)
{
    return SSL_set_fd(pstSsl, iFd);
}

INT SSL_UTL_Connect(IN VOID *pstSsl)
{
    INT iRet;
    INT iError;

    ERR_clear_error();
    
    iRet = SSL_connect(pstSsl);
    if (iRet > 0)
    {
        return SSL_UTL_E_NONE;
    }

    iError = SSL_get_error(pstSsl, iRet);

    switch (iError)
    {
        case SSL_ERROR_NONE:
        {
            iRet = SSL_UTL_E_NONE;
            break;
        }
        case SSL_ERROR_WANT_READ:
        {
            iRet = SSL_UTL_E_WANT_READ;
            break;
        }
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_X509_LOOKUP:
        {
            iRet = SSL_UTL_E_WANT_WRITE;
            break;
        }
        default:
        {
            iRet = SSL_UTL_E_ERROR;
            break;
        }
    }

    return iRet;
}

INT SSL_UTL_Accept(IN VOID *pstSsl)
{
    INT        iRet;
    INT        iError;

    ERR_clear_error();
    
    iRet = SSL_accept(pstSsl);
    if (iRet > 0)
    {
        return iRet;
    }

    iError = SSL_get_error(pstSsl, iRet);

    switch (iError)
    {
        case SSL_ERROR_NONE:
        {
            iRet = 1;
            break;
        }
        case SSL_ERROR_WANT_READ:   /* fall down */
        {
            iRet = SSL_UTL_E_WANT_READ;
            break;
        }
        case SSL_ERROR_WANT_WRITE:  /* fall down */
        case SSL_ERROR_WANT_X509_LOOKUP:
        {
            iRet = SSL_UTL_E_WANT_WRITE;
            break;
        }
        default:
        {
            iRet = SSL_UTL_E_ERROR;
            break;
        }
    }

    return iRet;
}

INT SSL_UTL_Read(IN VOID *pstSsl, IN VOID *pBuf, IN INT iNum)
{
    INT iRecvLen;
    INT iErrCode;
    INT iRet;

    ERR_clear_error();

    iRecvLen = SSL_read(pstSsl, pBuf, iNum);
    if (iRecvLen > 0)
    {
        return iRecvLen;
    }

    iErrCode = SSL_get_error(pstSsl, iRecvLen);

    switch (iErrCode)
    {
        case SSL_ERROR_NONE:
        {
            iRet = SOCKET_E_AGAIN;
            break;
        }
        case SSL_ERROR_WANT_READ:   /* fall down */
        case SSL_ERROR_WANT_WRITE:  /* fall down */
        case SSL_ERROR_WANT_X509_LOOKUP:
        {
            iRet = SOCKET_E_AGAIN;
            break;
        }
        case SSL_ERROR_ZERO_RETURN:
        {
            iRet = SOCKET_E_READ_PEER_CLOSE;
            break;
        }
        default:
        {
            iRet = SOCKET_E_ERR;
            break;
        }
    }

    return iRet;
}

INT SSL_UTL_Write(IN VOID *pstSsl, IN VOID *pBuf, IN INT iNum)
{
    INT iWriteLen;
    INT iErrCode;
    INT iRet;

    ERR_clear_error();

    iWriteLen = SSL_write(pstSsl, pBuf, iNum);
    if (iWriteLen > 0)
    {
        return iWriteLen;
    }

    iErrCode = SSL_get_error(pstSsl, iWriteLen);

    switch (iErrCode)
    {
        case SSL_ERROR_NONE:
        {
            iRet = SOCKET_E_AGAIN;
            break;
        }
        case SSL_ERROR_WANT_READ:   /* fall down */
        case SSL_ERROR_WANT_WRITE:  /* fall down */
        case SSL_ERROR_WANT_X509_LOOKUP:
        {
            iRet = SOCKET_E_AGAIN;
            break;
        }
        default:
        {
            iRet = SOCKET_E_ERR;
            break;
        }
    }

    return iRet;
}

INT SSL_UTL_Pending(IN VOID *pstSsl)
{
    return SSL_pending(pstSsl);
}

BS_STATUS SSL_UTL_SetAppData(IN VOID *pstSsl, IN CHAR *pcAppString)
{
    if (0 == SSL_set_app_data(pstSsl, pcAppString))
    {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS SSL_UTL_SetHostName(IN VOID *pstSsl, IN CHAR *pcHostName)
{
#ifdef SSL_set_tlsext_host_name
    if (0 == SSL_set_tlsext_host_name(pstSsl, pcHostName))
    {
        return BS_ERR;
    }
    return BS_OK;
#else
    return BS_NOT_SUPPORT;
#endif
}

BS_STATUS SSL_UTL_Renegotiate(IN VOID *pstSsl)
{
    if (0 == SSL_renegotiate(pstSsl))
    {
        return BS_ERR;
    }

    return BS_OK;
}

void * SSL_UTL_GetSslCert(void *ssl)
{
    return SSL_get_peer_certificate(ssl);
}
