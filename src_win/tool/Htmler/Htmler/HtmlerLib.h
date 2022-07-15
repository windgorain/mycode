#pragma once

#include <vector>
using namespace std;

class CKeyValue
{
public:
	CString sKey;
	CString sValue;
};

UINT HtmlerLib_AddMenuAction(IN CString sActions);
CString HtmlerLib_GetMenuAction(IN UINT uiIndex);
void HtmlerLib_ClearMenuAction(IN UINT uiIndex);
void HtmlerLib_SetExitActionList(IN CString sActions);
void HtmlerLib_ExitProcess();
void HtmlerLib_SetTray(IN HANDLE hTray);
HANDLE HtmlerLib_GetTray();
CHAR * Unicode2Ansi(IN wchar_t *pUnicode);
CHAR * Unicode2Utf8(IN wchar_t *pUnicode);
int StringHex2Int(IN CString sString);
CString StringUrlDecode(CString sString);
void StringSplit(CString inputString,char splitchar,CStringArray &saveStr);
void StringParseParams(CString inputString,char splitchar,char equelchar,vector<CKeyValue> &vec);
CString FindKeyValue(vector<CKeyValue> &vec, CString sKey);
CString HtmlerLib_DoActionOne(vector<CKeyValue> &vecActionLine);


