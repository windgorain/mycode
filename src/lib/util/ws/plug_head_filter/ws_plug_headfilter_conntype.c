/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-20
* Description: 
* History:     
******************************************************************************/
/*
HTTP版本号	客户端连接类型	客户端请求类型	服务器回复方式	服务器连接类型
1.0	        未指定	        动态	        close	        close
1.0	        未指定	        静态	        close	        close
1.0	        close	        动态	        close	        close
1.0	        close	        静态	        close	        close
1.0	        keep-alive	    动态	        close	        close
1.0	        keep-alive	    静态	        content-length	keep-alive   注:参见下面的备注信息
				
1.1	        未指定	        动态	        chunk	        keep-alive
1.1	        未指定	        静态	        content-length	keep-alive
1.1	        close	        动态	        close	        close
1.1	        close	        静态	        close	        close
1.1	        keep-alive	    动态	        chunk	        keep-alive
1.1	        keep-alive	    静态	        content-length	keep-alive
*/
/*
备注:
    互联网上，存在着大量简陋并过时的代理服务器在继续工作，它们很可能无法理解Connection——无论是请求报文还是响应
报文中的Connection。而代理服务器在遇到不认识的Header时，往往都会选择继续转发。大部分情况下这样做是对的，很多
使用HTTP协议的应用软件扩展了HTTP头部，如果代理不传输扩展字段，这些软件将无法工作。

    如果浏览器对这样的代理发送了Connection: Keep-Alive，那么结果会变得很复杂。这个Header会被不理解它的代理原封不
动的转给服务端，如果服务器也不能理解就还好，能理解就彻底杯具了。服务器并不知道Keep-Alive是由代理错误地转发而来，
它会认为代理希望建立持久连接。这很常见，服务端同意了，也返回一个Keep-Alive。同样，响应中的Keep-Alive也会被代理原
样返给浏览器，同时代理还会傻等服务器关闭连接——实际上，服务端已经按照Keep-Alive指示保持了连接，即时数据回传完成，
也不会关闭连接。另一方面，浏览器收到Keep-Alive之后，会复用之前的连接发送剩下的请求，但代理不认为这个连接上还会有
其他请求，请求被忽略。这样，浏览器会一直处于挂起状态，直到连接超时。

    这个问题最根本的原因是代理服务器转发了禁止转发的Header。但是要升级所有老旧的代理也不是件简单的事，所以浏览器
厂商和代理实现者协商了一个变通的方案：首先，显式给浏览器设置代理后，浏览器会把请求头中的Connection替换为Proxy-Connetion。
这样，对于老旧的代理，它不认识这个Header，会继续发给服务器，服务器也不认识，代理和服务器之间不会建立持久连接（不
能正确处理Connection的都是HTTP/1.0代理），服务器不返回Keep-Alive，代理和浏览器之间也不会建立持久连接。而对于新代
理，它可以理解Proxy-Connetion，会用Connection取代无意义的Proxy-Connection，并将其发送给服务器，以收到预期的效果。

    显然，如果浏览器并不知道连接中有老旧代理的存在，或者在老旧代理任意一侧有新代理的情况下，这种方案仍然无济于事。
所以有时候服务器也会选择彻底忽略HTTP/1.0的Keep-Alive特性：对于HTTP/1.0请求，从不使用持久连接，也从不返回Keep-Alive。

*/
#include "bs.h"
    
#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/time_utl.h"
#include "utl/ws_utl.h"

#include "../ws_def.h"
#include "../ws_conn.h"
#include "../ws_trans.h"
#include "../ws_event.h"
#include "../ws_context.h"


static VOID ws_plugconntype_PreBuildHead(IN WS_TRANS_S *pstTrans)
{
    HTTP_VERSION_E eVer;

    if (HTTP_CONNECTION_CLOSE == HTTP_GetConnection(pstTrans->hHttpHeadRequest))
    {
        HTTP_SetConnection(pstTrans->hHttpHeadReply, HTTP_CONNECTION_CLOSE);
        return;
    }

    if (NULL != HTTP_GetHeadField(pstTrans->hHttpHeadReply, HTTP_FIELD_CONTENT_LENGTH))
    {
        return;
    }

    eVer = HTTP_GetVersion(pstTrans->hHttpHeadRequest);
    if ((eVer == HTTP_VERSION_0_9)  || (eVer == HTTP_VERSION_1_0))
    {
        HTTP_SetConnection(pstTrans->hHttpHeadReply, HTTP_CONNECTION_CLOSE);
        return;
    }

    HTTP_SetConnection(pstTrans->hHttpHeadReply, HTTP_CONNECTION_CLOSE);

    return;
}

WS_EV_RET_E _WS_PlugConnType_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent)
{
    switch (uiEvent)
    {
        case WS_TRANS_EVENT_PRE_BUILD_HEAD:
        {
            (VOID) ws_plugconntype_PreBuildHead(pstTrans);
            break;
        }

        default:
        {
            break;
        }
    }

    return WS_EV_RET_CONTINUE;
}


