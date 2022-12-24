/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-10-14
* Description: 
* History:     
******************************************************************************/

#ifndef __HTML_PARSER_H_
#define __HTML_PARSER_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef VOID* HTML_PARSER_HANDLE;

typedef enum
{
    HTML_PARSER_DATA_TYPE_CONTENT = 0,
    HTML_PARSER_DATA_TYPE_TAG,
    HTML_PARSER_DATA_TYPE_JS_START,
    HTML_PARSER_DATA_TYPE_JS,
    HTML_PARSER_DATA_TYPE_JS_END,
    HTML_PARSER_DATA_TYPE_CSS_START,
    HTML_PARSER_DATA_TYPE_CSS,
    HTML_PARSER_DATA_TYPE_CSS_END,
}HTML_PARSE_DATA_TYPE_E;

#define HTML_PARSER_MAX_TAG_NAME_LEN 127
#define HTML_PARSER_MAX_ATTR_NAME_LEN 127
#define HTML_PARSER_MAX_ATTR_VALUE_LEN 8191

#define HTML_PARSER_TAG_FLAG_TAG_START       0x1 /* tag标签开始 */
#define HTML_PARSER_TAG_FLAG_TAG_END         0x2 /* tag标签结束 */
#define HTML_PARSER_TAG_FLAG_SELF_CLOSE      0x4 /* 自关闭 */
#define HTML_PARSER_TAG_FLAG_WITH_END_SLASH  0x8 /* 携带关闭标签的/字符 */

typedef struct
{
    UINT uiTagFlag;
    CHAR szTagName[HTML_PARSER_MAX_TAG_NAME_LEN + 1];
    CHAR szAttrName[HTML_PARSER_MAX_ATTR_NAME_LEN + 1];
    CHAR szAttrValue[HTML_PARSER_MAX_ATTR_VALUE_LEN + 1];
    USHORT usTagNameLen;
    USHORT usAttrNameLen;
    USHORT usAttrValueLen;
}HTML_PARSER_TAG_S;

typedef VOID (*PF_HTML_OUTPUT_FUNC)
(
    IN HTML_PARSE_DATA_TYPE_E enType,
    IN VOID *pData, /*content:LSTR_S; js: STRING_S; Tag: TAG_S */
    IN VOID *pUserContext
);

typedef enum
{
    HTML_PARSER_ELE_TYPE_DATA = 0,
    HTML_PARSER_ELE_TYPE_TAG
}HTML_PARSER_ELE_TYPE_E;

HTML_PARSER_HANDLE HTML_Parser_Create(IN PF_HTML_OUTPUT_FUNC pfOutput, IN VOID *pUserContext);
VOID HTML_Parser_Destroy(IN HTML_PARSER_HANDLE hPraser);
VOID HTML_Parser_Run
(
    IN HTML_PARSER_HANDLE hParser,
    IN CHAR *pcHtmlData,
    IN UINT uiHtmlDataLen
);
VOID HTML_Parser_End(IN HTML_PARSER_HANDLE hParser);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__HTML_PARSER_H_*/


