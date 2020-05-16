/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-9-23
* Description: 
* History:     
******************************************************************************/
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

typedef struct
{
    CHAR *pcFileExt;
    CHAR *pcContextType;
}_WS_CONTENT_TYPE_S;

STATIC _WS_CONTENT_TYPE_S g_astWsContentTypes[] =
{
    {"html",        "text/html"},
    {"htm",         "text/html"},
    {"shtml",       "text/html"},
    {"json",        "application/json"},
    {"css",         "text/css"},
    {"xml",         "text/xml"},
    {"gif",         "image/gif"},
    {"jpeg",        "image/jpeg"},
    {"jpg",         "image/jpeg"},
    {"js",          "application/x-javascript"},
    {"atom",        "application/atom+xml"},
    {"rss",         "application/rss+xml"},

    {"mml",         "text/mathml"},
    {"txt",         "text/plain"},
    {"jad",         "text/vnd.sun.j2me.app-descriptor"},
    {"wml",         "text/vnd.wap.wml"},
    {"htc",         "text/x-component"},

    {"png",         "image/png"},
    {"tif",         "image/tiff"},
    {"tiff",        "image/tiff"},
    {"wbmp",        "image/vnd.wap.wbmp"},
    {"ico",         "image/x-icon"},
    {"jng",         "image/x-jng"},
    {"bmp",         "image/bmp"},
    {"svg",         "image/svg+xml"},

    {"jar",         "application/java-archive"},
    {"war",         "application/java-archive"},
    {"ear",         "application/java-archive"},
    {"hqx",         "application/mac-binhex40"},
    {"doc",         "application/msword"},
    {"pdf",         "application/pdf"},
    {"ps",          "application/postscript"},
    {"eps",         "application/postscript"},
    {"ai",          "application/postscript"},
    {"rtf",         "application/rtf"},
    {"xls",         "application/vnd.ms-excel"},
    {"ppt",         "application/vnd.ms-powerpoint"},
    {"wmlc",        "application/vnd.wap.wmlc"},
    {"xhtml",       "application/vnd.wap.xhtml+xml"},
    {"kml",         "application/vnd.google-earth.kml+xml"},
    {"kmz",         "application/vnd.google-earth.kmz"},
    {"7z",          "application/x-7z-compressed"},
    {"cco",         "application/x-cocoa"},
    {"jardiff",     "application/x-java-archive-diff"},
    {"jnlp",        "application/x-java-jnlp-file"},
    {"run",         "application/x-makeself"},
    {"pl",          "application/x-perl"},
    {"pm",          "application/x-perl"},
    {"prc",         "application/x-pilot"},
    {"pdb",         "application/x-pilot"},
    {"rar",         "application/x-rar-compressed"},
    {"rpm",         "application/x-redhat-package-manager"},
    {"sea",         "application/x-sea"},
    {"swf",         "application/x-shockwave-flash"},
    {"sit",         "application/x-stuffit"},
    {"tcl",         "application/x-tcl"},
    {"tk",          "application/x-tcl"},
    {"der",         "application/x-x509-ca-cert"},
    {"pem",         "application/x-x509-ca-cert"},
    {"crt",         "application/x-x509-ca-cert"},
    {"xpi",         "application/x-xpinstall"},
    {"zip",         "application/zip"},

    {"bin",         "application/octet-stream"},
    {"exe",         "application/octet-stream"},
    {"dll",         "application/octet-stream"},
    {"deb",         "application/octet-stream"},
    {"dmg",         "application/octet-stream"},
    {"eot",         "application/octet-stream"},
    {"iso",         "application/octet-stream"},
    {"img",         "application/octet-stream"},
    {"msi",         "application/octet-stream"},
    {"msp",         "application/octet-stream"},
    {"msm",         "application/octet-stream"},
    {"mid",         "audio/midi"},
    {"midi",        "audio/midi"},
    {"kar",         "audio/midi"},
    {"mp3",         "audio/mpeg"},
    {"ra",          "audio/x-realaudio"},

    {"3gpp",        "video/3gpp"},
    {"3gp",         "video/3gpp"},
    {"mpeg",        "video/mpeg"},
    {"mpg",         "video/mpeg"},
    {"mov",         "video/quicktime"},
    {"flv",         "video/x-flv"},
    {"mng",         "video/x-mng"},
    {"asx",         "video/x-ms-asf"},
    {"asf",         "video/x-ms-asf"},
    {"wmv",         "video/x-ms-wmv"},
    {"avi",         "video/x-msvideo"}
};

static CHAR * ws_plugcontentype_GetContextType(IN WS_TRANS_S *pstTrans)
{
    WS_CONTEXT_HANDLE hContext;
    CHAR *pcRequestFile;
    CHAR *pcExternName;
    UINT uiIndex;
    CHAR *pcContextType = "text/html";

    hContext = pstTrans->hContext;
    if (NULL == hContext)
    {
        return pcContextType;
    }

    pcRequestFile = pstTrans->pcRequestFile;
    if (NULL == pcRequestFile)
    {
        return pcContextType;
    }

    pcExternName = FILE_GetExternNameFromPath(pcRequestFile, strlen(pcRequestFile));
    if (NULL == pcExternName)
    {
        return pcContextType;
    }

    for (uiIndex = 0; uiIndex<sizeof(g_astWsContentTypes)/sizeof(_WS_CONTENT_TYPE_S); uiIndex++)
    {
        if (stricmp(pcExternName, g_astWsContentTypes[uiIndex].pcFileExt) == 0)
        {
            pcContextType = g_astWsContentTypes[uiIndex].pcContextType;
            break;
        }
    }

    return pcContextType;
}

static VOID ws_plugcontenttype_PreBuildHead(IN WS_TRANS_S *pstTrans)
{
    CHAR *pcContentType;

    pcContentType = HTTP_GetHeadField(pstTrans->hHttpHeadReply, HTTP_FIELD_CONTENT_TYPE);
    if (NULL != pcContentType)
    {
        return;
    }

    pcContentType = ws_plugcontentype_GetContextType(pstTrans);

    HTTP_SetHeadField(pstTrans->hHttpHeadReply, HTTP_FIELD_CONTENT_TYPE, pcContentType);

    return;
}

WS_EV_RET_E _WS_PlugContentType_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent)
{
    switch (uiEvent)
    {
        case WS_TRANS_EVENT_PRE_BUILD_HEAD:
        {
            (VOID) ws_plugcontenttype_PreBuildHead(pstTrans);
            break;
        }

        default:
        {
            break;
        }
    }

    return WS_EV_RET_CONTINUE;
}

