/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-5-21
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/xml_parse.h"

#define _XML_PARSE_MAX_STATE_CHANGE_NUM 10


typedef enum
{
    XML_PARSE_STATE_INIT = 0,
    XML_PARSE_STATE_CONTENT,
    XML_PARSE_STATE_MARK,
    XML_PARSE_STATE_REMARK_1,   
    XML_PARSE_STATE_REMARK_2,   
    XML_PARSE_STATE_REMARK_3,   
    XML_PARSE_STATE_REMARK,
    XML_PARSE_STATE_REMARK_END_1,   
    XML_PARSE_STATE_REMARK_END_2,   
    XML_PARSE_STATE_MARK_NAME,
    XML_PARSE_STATE_MARK_NAME_END,
    XML_PARSE_STATE_MARK_KEY,
    XML_PARSE_STATE_MARK_KEY_END,
    XML_PARSE_STATE_MARK_KEY_EQUEL,     
    XML_PARSE_STATE_MARK_VALUE_SINGLE_QUOTE,           
    XML_PARSE_STATE_MARK_VALUE_SINGLE_QUOTE_VALUE,     
    XML_PARSE_STATE_MARK_VALUE_DOUBLE_QUOTE,           
    XML_PARSE_STATE_MARK_VALUE_DOUBLE_QUOTE_VALUE,     
    XML_PARSE_STATE_MARK_VALUE_QUOTE_END,              
    XML_PARSE_STATE_MARK_VALUE,
    XML_PARSE_STATE_MARK_VALUE_END,
    XML_PARSE_STATE_MARK_END,       
    XML_PARSE_STATE_BF_END_MARK,       

    XML_PARSE_STATE_ERROR
}_XML_PARSE_STATE_E;

typedef enum
{
    _XML_PARSE_ACT_NONE = 0,    
    _XML_PARSE_ACT_STR1,        
    _XML_PARSE_ACT_LEN1,        
    _XML_PARSE_ACT_STR2,        
    _XML_PARSE_ACT_LEN2,        
    _XML_PARSE_ACT_L2_0,        
}_XML_PARSE_ACT_E;

typedef struct
{
    CHAR cChar;                     
    _XML_PARSE_STATE_E eToState;    
    _XML_PARSE_ACT_E eAction;       
    XML_TYPE_E eType;               
}_XML_PARSE_STATE_CHANGE_TO_S;

typedef struct
{
    _XML_PARSE_STATE_E eState;
    _XML_PARSE_STATE_CHANGE_TO_S astStateChange[_XML_PARSE_MAX_STATE_CHANGE_NUM];
}_XML_PARSE_STATE_MACHINE_S;


static _XML_PARSE_STATE_MACHINE_S g_astXmlParseStateMachine[] =
{
    {XML_PARSE_STATE_INIT,
        {{'<', XML_PARSE_STATE_MARK, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},     
        {0,    XML_PARSE_STATE_INIT, _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_CONTENT,
        {{'<', XML_PARSE_STATE_MARK, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_CONTENT, _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_MARK,
        {{'>', XML_PARSE_STATE_MARK_END,   _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'/', XML_PARSE_STATE_BF_END_MARK, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {' ', XML_PARSE_STATE_MARK,        _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\t', XML_PARSE_STATE_MARK,       _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\r', XML_PARSE_STATE_MARK,       _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\n', XML_PARSE_STATE_MARK,       _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'!', XML_PARSE_STATE_REMARK_1,    _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_MARK_NAME,     _XML_PARSE_ACT_STR1, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_REMARK_1,
        {{'-', XML_PARSE_STATE_REMARK_2, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_ERROR,       _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_REMARK_2,
        {{'-', XML_PARSE_STATE_REMARK_3, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_ERROR,       _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_REMARK_3,
        {{'-', XML_PARSE_STATE_REMARK_END_1, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'>', XML_PARSE_STATE_ERROR,         _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_REMARK,          _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_REMARK,
        {{'-', XML_PARSE_STATE_REMARK_END_1, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'>', XML_PARSE_STATE_ERROR,         _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_REMARK,          _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_REMARK_END_1,
        {{'-', XML_PARSE_STATE_REMARK_END_2, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'>', XML_PARSE_STATE_ERROR,         _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_REMARK,          _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_REMARK_END_2,
        {{'>', XML_PARSE_STATE_MARK_END,     _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_REMARK,          _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_MARK_NAME,
        {{' ', XML_PARSE_STATE_MARK_NAME_END, _XML_PARSE_ACT_LEN1, XML_TYPE_MARK_NAME},
        {'\t', XML_PARSE_STATE_MARK_NAME_END, _XML_PARSE_ACT_LEN1, XML_TYPE_MARK_NAME},
        {'\r', XML_PARSE_STATE_MARK_NAME_END, _XML_PARSE_ACT_LEN1, XML_TYPE_MARK_NAME},
        {'\n', XML_PARSE_STATE_MARK_NAME_END, _XML_PARSE_ACT_LEN1, XML_TYPE_MARK_NAME},
        {'/', XML_PARSE_STATE_BF_END_MARK,    _XML_PARSE_ACT_LEN1, XML_TYPE_MARK_NAME},
        {'>', XML_PARSE_STATE_MARK_END,       _XML_PARSE_ACT_LEN1, XML_TYPE_MARK_NAME},
        {0, XML_PARSE_STATE_MARK_NAME,        _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_MARK_NAME_END,
        {{' ', XML_PARSE_STATE_MARK_NAME_END, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\t', XML_PARSE_STATE_MARK_NAME_END, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\r', XML_PARSE_STATE_MARK_NAME_END, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\n', XML_PARSE_STATE_MARK_NAME_END, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'/', XML_PARSE_STATE_BF_END_MARK,    _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'>', XML_PARSE_STATE_MARK_END,       _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'=', XML_PARSE_STATE_ERROR,          _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_MARK_KEY,         _XML_PARSE_ACT_STR1, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_MARK_KEY,
        {{' ', XML_PARSE_STATE_MARK_KEY_END,  _XML_PARSE_ACT_LEN1, XML_TYPE_NONE},
        {'\t', XML_PARSE_STATE_MARK_KEY_END,  _XML_PARSE_ACT_LEN1, XML_TYPE_NONE},
        {'\r', XML_PARSE_STATE_MARK_KEY_END,  _XML_PARSE_ACT_LEN1, XML_TYPE_NONE},
        {'\n', XML_PARSE_STATE_MARK_KEY_END,  _XML_PARSE_ACT_LEN1, XML_TYPE_NONE},
        {'=', XML_PARSE_STATE_MARK_KEY_EQUEL, _XML_PARSE_ACT_LEN1, XML_TYPE_NONE},
        {'/', XML_PARSE_STATE_ERROR,          _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'>', XML_PARSE_STATE_ERROR,          _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_MARK_KEY,         _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_MARK_KEY_END,
        {{' ', XML_PARSE_STATE_MARK_KEY_END,  _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\t', XML_PARSE_STATE_MARK_KEY_END,  _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\r', XML_PARSE_STATE_MARK_KEY_END,  _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\n', XML_PARSE_STATE_MARK_KEY_END,  _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'=', XML_PARSE_STATE_MARK_KEY_EQUEL, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_ERROR,            _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_MARK_KEY_EQUEL,
        {{' ', XML_PARSE_STATE_MARK_KEY_EQUEL,          _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\t', XML_PARSE_STATE_MARK_KEY_EQUEL,          _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\r', XML_PARSE_STATE_MARK_KEY_EQUEL,          _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\n', XML_PARSE_STATE_MARK_KEY_EQUEL,          _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\'', XML_PARSE_STATE_MARK_VALUE_SINGLE_QUOTE, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\"', XML_PARSE_STATE_MARK_VALUE_DOUBLE_QUOTE, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'=', XML_PARSE_STATE_ERROR,                    _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'/', XML_PARSE_STATE_ERROR,                    _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'>', XML_PARSE_STATE_ERROR,                    _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_MARK_VALUE,                 _XML_PARSE_ACT_STR2, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_MARK_VALUE_SINGLE_QUOTE,
        {{'\'', XML_PARSE_STATE_MARK_VALUE_QUOTE_END,      _XML_PARSE_ACT_L2_0, XML_TYPE_MARK_KEYVALUE},
        {0, XML_PARSE_STATE_MARK_VALUE_SINGLE_QUOTE_VALUE, _XML_PARSE_ACT_STR2, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_MARK_VALUE_SINGLE_QUOTE_VALUE,
        {{'\'', XML_PARSE_STATE_MARK_VALUE_QUOTE_END,      _XML_PARSE_ACT_LEN2, XML_TYPE_MARK_KEYVALUE},
        {0, XML_PARSE_STATE_MARK_VALUE_SINGLE_QUOTE_VALUE, _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_MARK_VALUE_DOUBLE_QUOTE,
        {{'\"', XML_PARSE_STATE_MARK_VALUE_QUOTE_END,      _XML_PARSE_ACT_L2_0, XML_TYPE_MARK_KEYVALUE},
        {0, XML_PARSE_STATE_MARK_VALUE_DOUBLE_QUOTE_VALUE, _XML_PARSE_ACT_STR2, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_MARK_VALUE_DOUBLE_QUOTE_VALUE,
        {{'\"', XML_PARSE_STATE_MARK_VALUE_QUOTE_END,      _XML_PARSE_ACT_LEN2, XML_TYPE_MARK_KEYVALUE},
        {0, XML_PARSE_STATE_MARK_VALUE_DOUBLE_QUOTE_VALUE, _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_MARK_VALUE_QUOTE_END,
        {{'/', XML_PARSE_STATE_BF_END_MARK,    _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'>', XML_PARSE_STATE_MARK_END,        _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {' ', XML_PARSE_STATE_MARK_VALUE_END,  _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\t', XML_PARSE_STATE_MARK_VALUE_END, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\r', XML_PARSE_STATE_MARK_VALUE_END, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\n', XML_PARSE_STATE_MARK_VALUE_END, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_ERROR,             _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_MARK_VALUE,
        {{'/', XML_PARSE_STATE_BF_END_MARK,    _XML_PARSE_ACT_LEN2, XML_TYPE_MARK_KEYVALUE},
        {'>', XML_PARSE_STATE_MARK_END,        _XML_PARSE_ACT_LEN2, XML_TYPE_MARK_KEYVALUE},
        {' ', XML_PARSE_STATE_MARK_VALUE_END,  _XML_PARSE_ACT_LEN2, XML_TYPE_MARK_KEYVALUE},
        {'\t', XML_PARSE_STATE_MARK_VALUE_END, _XML_PARSE_ACT_LEN2, XML_TYPE_MARK_KEYVALUE},
        {'\r', XML_PARSE_STATE_MARK_VALUE_END, _XML_PARSE_ACT_LEN2, XML_TYPE_MARK_KEYVALUE},
        {'\n', XML_PARSE_STATE_MARK_VALUE_END, _XML_PARSE_ACT_LEN2, XML_TYPE_MARK_KEYVALUE},
        {0, XML_PARSE_STATE_MARK_VALUE,        _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_MARK_VALUE_END,
        {{'/', XML_PARSE_STATE_BF_END_MARK,     _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'>', XML_PARSE_STATE_MARK_END,         _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {' ', XML_PARSE_STATE_MARK_VALUE_END,   _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\t', XML_PARSE_STATE_MARK_VALUE_END,  _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\r', XML_PARSE_STATE_MARK_VALUE_END,  _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {'\n', XML_PARSE_STATE_MARK_VALUE_END,  _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_MARK_KEY,           _XML_PARSE_ACT_STR1, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_MARK_END,
        {{'<', XML_PARSE_STATE_MARK, _XML_PARSE_ACT_NONE, XML_TYPE_NONE},
        {0, XML_PARSE_STATE_CONTENT, _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}},
    {XML_PARSE_STATE_BF_END_MARK,
        {{'>', XML_PARSE_STATE_MARK_END, _XML_PARSE_ACT_NONE, XML_TYPE_END_MARK},
        {0, XML_PARSE_STATE_BF_END_MARK, _XML_PARSE_ACT_NONE, XML_TYPE_NONE}}}
};

#ifdef IN_DEBUG

static BOOL_T _XML_PARSE_IsStateMachineValie()
{
    UINT i;
    for (i=0; i<sizeof(g_astXmlParseStateMachine)/sizeof(_XML_PARSE_STATE_MACHINE_S); i++)
    {
        if ((UINT)g_astXmlParseStateMachine[i].eState != i)
        {
            BS_DBGASSERT(0);
            return FALSE;
        }
    }

    return TRUE;
}
#endif

BS_STATUS XML_Parse(IN CHAR *pszContent, IN PF_XML_PARSE_FUNC pfFunc, IN HANDLE hUserHandle)
{
    CHAR *pt;
    CHAR cChar;
    UINT i;
    _XML_PARSE_STATE_CHANGE_TO_S *pstChangeTo;
    _XML_PARSE_STATE_E eState = XML_PARSE_STATE_INIT;
    XML_PARSE_S stParse;
    BS_STATUS eRet;

#ifdef IN_DEBUG
    
    _XML_PARSE_IsStateMachineValie();
#endif

    Mem_Zero(&stParse, sizeof(XML_PARSE_S));

    pt = pszContent;

    while (*pt != '\0')
    {
        cChar = *pt;

        for (i=0; i<_XML_PARSE_MAX_STATE_CHANGE_NUM; i++)
        {
            pstChangeTo = &g_astXmlParseStateMachine[eState].astStateChange[i];

            if ((pstChangeTo->cChar == cChar) || (pstChangeTo->cChar == 0))
            {
                eState = pstChangeTo->eToState;

                if (eState == XML_PARSE_STATE_ERROR)
                {
                    BS_DBGASSERT(0);
                    return BS_ERR;
                }

                switch (pstChangeTo->eAction)
                {
                    case _XML_PARSE_ACT_STR1:
                        stParse.pszStr1 = pt;
                        break;
                    case _XML_PARSE_ACT_LEN1:
                        stParse.uiStr1Len = pt - stParse.pszStr1;
                        break;
                    case _XML_PARSE_ACT_STR2:
                        stParse.pszStr2 = pt;
                        break;
                    case _XML_PARSE_ACT_LEN2:
                        stParse.uiStr2Len = pt - stParse.pszStr2;
                        break;
                    case _XML_PARSE_ACT_L2_0:
                        stParse.pszStr2 = pt;
                        stParse.uiStr2Len = 0;
                        break;
                    default:
                        break;
                }

                if (pstChangeTo->eType != XML_TYPE_NONE)
                {
                    stParse.eType = pstChangeTo->eType;
                    eRet = pfFunc(&stParse, hUserHandle);
                    if (eRet != BS_OK)
                    {
                        return eRet;
                    }
                }

                break;
            }
        }

        pt++;
    }

	return BS_OK;
}

