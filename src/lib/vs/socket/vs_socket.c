/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-2-25
* Description: 
* History:     
******************************************************************************/

static VS_SOCKET_S * vs_soalloc()
{
    VS_SOCKET_S *pstSo;

    pstSo = MEM_ZMalloc(sizeof(VS_SOCKET_S));
    if (pstSo == NULL)
    {
        return (NULL);
    }

    DLL_INIT(&pstSo->stCompList);
    DLL_INIT(&pstSo->stInCompList);

    return pstSo;
}

static void vs_sodealloc(IN SOCKET_S *pstSocket)
{
    MEM_Free(pstSocket);
}

static SOCKET_S * vs_socreate(IN UINT uiFamily, IN USHORT usType, IN USHORT usProtocol)
{
    VS_PROTOSW_S *pstProto;
    VS_SOCKET_S *so;
    int error;

    if (usProtocol)
    {
        pstProto = VS_DOMAIN_FindProto(uiFamily, usProtocol, usType);
    }
    else
    {
        pstProto = VS_DOMAIN_FindType(uiFamily, usType);
    }

    if ((pstProto == NULL)
       || (pstProto->pstUsrRequest->pfAttach == NULL)
       || (pstProto->pstUsrRequest->pfAttach == (VOID*)pru_notsupp))
    {
        return NULL;
    }

    so = vs_soalloc();
    if (so == NULL)
    {
        return NULL;
    }

    so->usType = usType;
    so->pstProto = pstProto;

    error = pstProto->pstUsrRequest->pfAttach(so, usProtocol);
    if (error)
    {
        vs_sodealloc(so);
        return NULL;
    }

    return so;
}

static INT vs_soclose1(IN VS_SOCKET_S *so)
{
    int error = 0;

    if ((so->usState & SS_ISCONNECTED) == 0)
    {
        return 0;
    }

    if ((so->usState & SS_ISDISCONNECTING) == 0)
    {
        error = sodisconnect(so);
        if (error != 0)
        {
            return error;
        }
    }

    if (so->uiOptions & SO_LINGER)
    {
        if ((so->usState & SS_ISDISCONNECTING)
            &&((so->pstFile) && (so->pstFile->uiFlag & O_NONBLOCK)))
        {
            return 0;
        }

        
        SOCK_LOCK(so);
        while (so->so_state & SS_ISCONNECTED)
        {
            if (BS_TIME_OUT == so_msleep(so->so_wait, SOCK_SEM(so), so->so_linger))
            {
                error = EINTR;
            }

            if (error)
            {
                break;
            }
        }
        SOCK_UNLOCK(so);
    }

    return error;
}

static INT vs_soclose(IN VS_SOCKET_S *so)
{
    int error = 0;

    error = vs_soclose1(so);

    if (so->so_proto->pr_usrreqs->pru_close != NULL)
    {
        (*so->so_proto->pr_usrreqs->pru_close)(so);
    }

    if (so->so_options & SO_ACCEPTCONN)
    {
        SOCKET_S *sp;
        ACCEPT_LOCK();
        while ((sp = TAILQ_FIRST(&so->so_incomp)) != NULL)
        {
            TAILQ_REMOVE(&so->so_incomp, sp, so_list);
            so->so_incqlen--;
            sp->so_qstate &= ~SQ_INCOMP;
            sp->so_head = NULL;
            ACCEPT_UNLOCK();
            soabort(sp);
            ACCEPT_LOCK();
        }

        while ((sp = TAILQ_FIRST(&so->so_comp)) != NULL)
        {
            TAILQ_REMOVE(&so->so_comp, sp, so_list);
            so->so_qlen--;
            sp->so_qstate &= ~SQ_COMP;
            sp->so_head = NULL;
            ACCEPT_UNLOCK();
            soabort(sp);
            ACCEPT_LOCK();
        }
        ACCEPT_UNLOCK();
    }
    ACCEPT_LOCK();
    SOCK_LOCK(so);
    BS_DBGASSERT((so->so_state & SS_NOFDREF) == 0);
    so->so_state |= SS_NOFDREF;
    so->so_file = NULL;
    sorele(so);
    return (error);
}

INT VS_Socket(IN UINT uiFamily, IN USHORT usType, IN USHORT usProtocol)
{
    INT iFd;
    VS_FD_S *pstFp;
    VS_SOCKET_S *pstSocket;

    iFd = VS_FD_Alloc(&pstFp);
    if (iFd < 0)
    {
        return iFd;
    }

    pstSocket = vs_socreate(uiFamily, usType, usProtocol);
    if (pstSocket == NULL)
    {
        VS_FD_Close(pstFp);
        iFd = -1;
    }
    else
    {
        VS_FD_Attach(pstFp, pstSocket);
    }

    return iFd;
}

int VS_Close(IN INT iFd)
{
    SOCKET_S *so;
    UPIC_FD_S *pstFp;

    pstFp = VS_FD_GetFp(iFd);
    if (pstFp == NULL)
    {
        return EBADF;
    }

    so = VS_FD_GetSocket(pstFp);

    return soclose(so);
}
