/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-9-23
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mac_table.h"
#include "utl/ippool_utl.h"
#include "utl/udp_utl.h"
#include "utl/dhcp_utl.h"

INT DHCP_GetDHCPMsgType (IN DHCP_HEAD_S *pstDhcpHead, IN UINT ulOptLen)
{
    const UCHAR *p = (UCHAR *) (pstDhcpHead + 1);
    UINT i;

    for (i = 0; i < ulOptLen; ++i)
    {
        const UCHAR type = p[i];
        const int room = ulOptLen - i - 1;
      
        if (type == DHCP_END)           // didn't find what we were looking for
        {
            return -1;
        }
        else if (type == DHCP_PAD)      // no-operation
        {
            ;
        }
        else if (type == DHCP_MSG_TYPE) // what we are looking for
        {
            if (room >= 2)
            {
                if (p[i+1] == 1)        // message length should be 1
                {
                    return p[i+2];        // return message type
                }
            }
            return -1;
        }
        else                            // some other message
        {
            if (room >= 1)
            {
                const int len = p[i+1]; // get message length
                i += (len + 1);         // advance to next message
            }
        }
    }

    return -1;
}

/* 得到要申请的IP地址, 网络序 */
UINT DHCP_GetRequestIpByDhcpRequest (IN DHCP_HEAD_S *pstDhcpHead, IN UINT ulOptLen)
{
    const UCHAR *p = (UCHAR *) (pstDhcpHead + 1);
    UINT i;
    UINT ulIp;

    if (pstDhcpHead->ulClientIp != 0)
    {
        return pstDhcpHead->ulClientIp;
    }

    for (i = 0; i < ulOptLen; ++i)
    {
        const UCHAR type = p[i];
        const int room = ulOptLen - i - 1;
      
        if (type == DHCP_END)           // didn't find what we were looking for
        {
            return 0;
        }
        else if (type == DHCP_PAD)      // no-operation
        {
            ;
        }
        else if (type == DHCP_REQUEST_IP) // what we are looking for
        {
            if (room >= 5)
            {
                if (p[i+1] == 4)        // message length should be 4
                {
                    ulIp = *(UINT*) (&p[i+2]);
                    return ulIp;
                }
            }
            return 0;
        }
        else                            // some other message
        {
            if (room >= 1)
            {
                const int len = p[i+1]; // get message length
                i += (len + 1);         // advance to next message
            }
        }
    }

    return 0;
}

UINT DHCP_SetOpt0 (UCHAR *pucOpt, int type, IN UINT ulLen)
{
    VNET_DHCP_OPT0 stOpt;

    if (sizeof(stOpt) > ulLen)
    {
        BS_DBGASSERT (0);
        return 0;
    }

    stOpt.type = (UCHAR) type;

    MEM_Copy (pucOpt, &stOpt, sizeof(stOpt));

    return sizeof (stOpt);
}


UINT DHCP_SetOpt8 (UCHAR *pucOpt, int type, UCHAR data, IN UINT ulLen)
{
  VNET_DHCP_OPT8 stOpt;

  if (sizeof(stOpt) > ulLen)
  {
      BS_DBGASSERT (0);
      return 0;
  }

  
  stOpt.type = (UCHAR) type;
  stOpt.len = sizeof (stOpt.data);
  stOpt.data = (UCHAR) data;
  MEM_Copy (pucOpt, &stOpt, sizeof(stOpt));

  return sizeof (stOpt);
}

UINT DHCP_SetOpt32 (UCHAR *pucOpt, int type, UINT data, IN UINT ulLen)
{
  VNET_DHCP_OPT32 stOpt;

  if (sizeof(stOpt) > ulLen)
  {
      BS_DBGASSERT (0);
      return 0;
  }

  stOpt.type = (UCHAR) type;
  stOpt.len = sizeof (stOpt.data);
  stOpt.data = data;
  MEM_Copy (pucOpt, &stOpt, sizeof(stOpt));

  return sizeof (stOpt);
}

