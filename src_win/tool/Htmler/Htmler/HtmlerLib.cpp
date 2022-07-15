#include "stdafx.h"
#include "Htmler.h"
#include "HtmlerDlg.h"
#include "HtmlerLib.h"
#include "DllList.h"
#include "afxdialogex.h"

#include "bs.h"
#include "utl/str_conversion.h"
#include "utl/tray_utl.h"

#define HTMLER_LIB_MENU_BASE_INDEX (WM_USER + 200)  /* 菜单ID的基线 */

static HANDLE g_hHtmlerLibTrayId = NULL;    /* Tray实例 */
static CString g_sExitActions = _T("");  /* 进程退出之前要执行的命令 */
static CStringArray g_astHtmlerLibMenuActionArray;
static CDllList g_DllList;

UINT HtmlerLib_AddMenuAction(IN CString sActions)
{
	UINT uiIndex;

	for (uiIndex=0; uiIndex<(UINT)g_astHtmlerLibMenuActionArray.GetSize(); uiIndex++)
	{
		if (g_astHtmlerLibMenuActionArray[uiIndex] == "")
		{
			g_astHtmlerLibMenuActionArray[uiIndex] = sActions;
			return uiIndex + HTMLER_LIB_MENU_BASE_INDEX;
		}
	}

	uiIndex = g_astHtmlerLibMenuActionArray.GetSize();
	g_astHtmlerLibMenuActionArray.Add(sActions);

	return uiIndex + HTMLER_LIB_MENU_BASE_INDEX;
}

CString HtmlerLib_GetMenuAction(IN UINT uiIndex)
{
	if (uiIndex < HTMLER_LIB_MENU_BASE_INDEX)
	{
		return _T("");
	}

	uiIndex -= HTMLER_LIB_MENU_BASE_INDEX;

	if (uiIndex >= (UINT)g_astHtmlerLibMenuActionArray.GetSize())
	{
		return _T("");
	}

	return g_astHtmlerLibMenuActionArray[uiIndex];
}

void HtmlerLib_ClearMenuAction(IN UINT uiIndex)
{
	if (uiIndex < HTMLER_LIB_MENU_BASE_INDEX)
	{
		return;
	}

	uiIndex -= HTMLER_LIB_MENU_BASE_INDEX;

	if (uiIndex >= (UINT)g_astHtmlerLibMenuActionArray.GetSize())
	{
		return;
	}

	g_astHtmlerLibMenuActionArray[uiIndex] = _T("");

	return;
}

void HtmlerLib_SetExitActionList(IN CString sActions)
{
	g_sExitActions += ";";
	g_sExitActions += sActions;
}

void HtmlerLib_ExitProcess()
{
	CHtmlerDlg *pMainHtmler;

	pMainHtmler = (CHtmlerDlg*)AfxGetMainWnd();
	pMainHtmler->my_DoActionList(g_sExitActions);

	if (g_hHtmlerLibTrayId != NULL)
	{
		TRAY_Delete(g_hHtmlerLibTrayId);
		g_hHtmlerLibTrayId = NULL;
	}
}

void HtmlerLib_SetTray(IN HANDLE hTray)
{
	g_hHtmlerLibTrayId = hTray;
}

HANDLE HtmlerLib_GetTray()
{
	return g_hHtmlerLibTrayId;
}

CHAR * Unicode2Ansi(IN wchar_t *pUnicode)
{
	static CHAR szAnsi[1024];

	if (0 == STR_UnicodeToAnsi(pUnicode, szAnsi, 1024))
	{
		return NULL;
	}

	return szAnsi;
}

CHAR * Unicode2Utf8(IN wchar_t *pUnicode)
{
	static CHAR szAnsi[1024];

	if (0 == STR_UnicodeToUtf8(pUnicode, szAnsi, 1024))
	{
		return NULL;
	}

	return szAnsi;
}

int StringHex2Int(IN CString sString)
{
	int iRet = 0;

	for (int i=0; i<sString.GetLength(); i++)
	{
		iRet = iRet * 16;

		if ((sString[i] <='9') && (sString[i] >= '0'))
		{
			iRet += sString[i] - '0';
		}
		else if ((sString[i] <='F') && (sString[i] >= 'A'))
		{
			iRet += sString[i] - 'A' + 10;
		}
		else if ((sString[i] <='f') && (sString[i] >= 'a'))
		{
			iRet += sString[i] - 'a' + 10;
		}
		else
		{
			return -1;
		}
	}

	return iRet;
}

CString StringUrlDecode(CString sString)
{
	CString sDest = _T("");

	for (int i=0; i<sString.GetLength(); i++)
	{
		if (sString[i] != '%')
		{
			sDest += sString[i];
			continue;
		}

		CString sTmp = sString.Mid(i+1, 2);
		int iNum = StringHex2Int(sTmp);
		sTmp.Format(_T("%c"), iNum);
		sDest += sTmp;
		i+=2;
	}

	return sDest;
}

void StringSplit(CString inputString,char splitchar,CStringArray &saveStr)
{
    CString outStr;
    int startIndex = 0;
	int iLen;
	CString sSrcString;

	sSrcString = inputString.Trim();

	if (sSrcString.GetLength() == 0)
	{
		return;
	}

    for (int i=0;i<sSrcString.GetLength();i++)
    {
		if (sSrcString[i] == splitchar)
		{
			iLen = i-startIndex;
			if (iLen != 0)
			{
				outStr = sSrcString.Mid(startIndex,iLen);
				outStr = outStr.Trim();
				if (outStr.GetLength() > 0)
				{
					saveStr.Add(outStr);
				}
			}
			startIndex = i+1;
		}
    }

	iLen = sSrcString.GetLength() - startIndex;
	if (iLen > 0)
	{
		outStr = sSrcString.Mid(startIndex,iLen);
		outStr = outStr.Trim();
		if (outStr.GetLength() > 0)
		{
			saveStr.Add(outStr);
		}
	}

	return;
}

void StringParseParams(CString inputString,char splitchar,char equelchar,vector<CKeyValue> &vec)
{
    CString sKeyValue;
	CStringArray stStringArray;
	int iIndex;

	StringSplit(inputString, splitchar, stStringArray);

	if (stStringArray.GetSize() == 0)
	{
		return;
	}

    for (int i =0; i< stStringArray.GetSize(); i++)
    {
		sKeyValue = stStringArray.GetAt(i);
		iIndex = sKeyValue.Find(equelchar);
		if (iIndex == 0)
		{
			continue;
		}

		CKeyValue stKeyValue;

		if (iIndex < 0)
		{
			stKeyValue.sKey = sKeyValue;
			stKeyValue.sValue = _T("");
		}
		else
		{
			stKeyValue.sKey = sKeyValue.Mid(0, iIndex).Trim();
			stKeyValue.sValue = sKeyValue.Mid(iIndex+1, (sKeyValue.GetLength() - iIndex) - 1).Trim();
			stKeyValue.sValue = StringUrlDecode(stKeyValue.sValue);
		}

		if (stKeyValue.sKey.GetLength() > 0)
		{
			vec.push_back(stKeyValue);
		}
	}
}

CHAR * String2Char(CString sString)
{
	int len;

	len = WideCharToMultiByte(CP_UTF8, 0, sString, -1, NULL, 0, NULL, FALSE);
	if (len <= 0)
	{
		return NULL;
	}
	
	char *pcFunc = new char[len];
	WideCharToMultiByte(CP_UTF8, 0, sString, -1, pcFunc, len, NULL, FALSE);

	return pcFunc;
}

CString FindKeyValue(vector<CKeyValue> &vec, CString sKey)
{
	for (unsigned int i=0; i<vec.size(); i++)
	{
		if (vec[i].sKey == sKey)
		{
			return vec[i].sValue;
		}
	}

	return _T("");
}

static void htmlerlib_CallJsFunc(CString sFunctionName, CString sParam)
{
	IHTMLDocument2 *pDoc;
	HRESULT hr;
	IDispatch *pDispScript;
	DISPID dispid;
	CComVariant *pParams;
	
	hr = ((CHtmlerDlg*)AfxGetMainWnd())->GetDHtmlDocument(&pDoc);

	hr = pDoc->get_Script(&pDispScript);
	if (FAILED(hr))
	{
		return;
	}

	CComBSTR objBstrValue = sFunctionName;
	BSTR bstrValue = objBstrValue.Copy();
	OLECHAR *pszFunct = bstrValue;

	hr = pDispScript->GetIDsOfNames(IID_NULL, &pszFunct, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
	if (S_OK != hr)
	{
		pDispScript->Release();
		return;
	}

	pParams = new CComVariant[1];
	DISPPARAMS dispParams = {pParams, NULL, 1, 0};

	pParams[0].vt = VT_BSTR;

	objBstrValue = sParam;
	objBstrValue.CopyTo(&pParams[0].bstrVal);

	EXCEPINFO except;
	memset(&except, 0, sizeof(except));
	UINT nArgErr = 0xffffffff;
	
	hr = pDispScript->Invoke(dispid, IID_NULL, 0, DISPATCH_METHOD, &dispParams, pParams, &except, &nArgErr);

	pDispScript->Release();
	return;
}

static void htmlerlib_DoActionStringOne(CString sActionLine)
{
	vector<CKeyValue> vecActionLine;
	CString sRet = _T("");
	CString sAction;

	StringParseParams(sActionLine, ',', '=', vecActionLine);

	HtmlerLib_DoActionOne(vecActionLine);
}

static DWORD htmlerlib_ThreadFunc(LPVOID  pParam)
{
    CString *psAction = (CString *)pParam;
	CString sActionList;
	CStringArray astActionArray;

	sActionList = *psAction;
	delete psAction;

	StringSplit(sActionList, ';', astActionArray);

	if (astActionArray.GetSize() == 0)
	{
		return 0;
	}

	for (int i =0; i< astActionArray.GetSize(); i++)
	{
		htmlerlib_DoActionStringOne(astActionArray[i]);
	}

	return 0;
}


CString HtmlerLib_DoActionOne(vector<CKeyValue> &vecActionLine)
{
	CString sRet = _T("");
	CString sAction;

	sAction = FindKeyValue(vecActionLine, _T("Action"));

	if (sAction == "CheckSupport")	/* 检查是否支持 */
	{
		sRet.Format(_T("{\"Support\": 1}"));
	}
	else if (sAction == "CallJsFunction")
	{
		CString sFunction = FindKeyValue(vecActionLine, _T("Function"));
		CString sParam = FindKeyValue(vecActionLine, _T("Param"));
		htmlerlib_CallJsFunc(sFunction, sParam);
	}
	else if (sAction == "SetExitAction")
	{
		HtmlerLib_SetExitActionList(FindKeyValue(vecActionLine, _T("DoAction")));
	}
	else if (sAction == "MsgBox")
	{
		MessageBox(NULL, FindKeyValue(vecActionLine, _T("Text")), FindKeyValue(vecActionLine, _T("Caption")), MB_OK);
	}
	else if (sAction == "Exit")
	{
		HtmlerLib_ExitProcess();
		exit(0);
	}
	else if (sAction == "CreateModHtmler")
	{
		CHtmlerDlg dlgTmp;
		dlgTmp.my_SetIndex(FindKeyValue(vecActionLine, _T("URL")));
		dlgTmp.my_SetInitDoAction(FindKeyValue(vecActionLine, _T("DoAction")));
		dlgTmp.DoModal();
	}
	else if (sAction == "CreateModlessHtmler")
	{
		((CHtmlerDlg*)AfxGetMainWnd())->my_CreateModelessHtmler(vecActionLine);
	}
	else if (sAction == "CreateThread")
	{
        CString sDoAction = FindKeyValue(vecActionLine, _T("DoAction"));
        CString *psDoAction = new CString(sDoAction);
        
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)htmlerlib_ThreadFunc, psDoAction, NULL, NULL);
	}
	else if (sAction == "CreateTray")
	{
		((CHtmlerDlg*)AfxGetMainWnd())->my_CreateTray(vecActionLine);
	}
	else if (sAction == "ReadFile")
	{
		CString sFile = FindKeyValue(vecActionLine, _T("File"));
		CFile file;
		DWORD fileLen;

		if (FALSE != file.Open(sFile,CFile::modeRead))
		{
			fileLen = (DWORD)file.GetLength();
			if (fileLen != 0)
			{
				unsigned char *pucFileContent = new unsigned char[fileLen + 1];
				file.Read(pucFileContent, fileLen);
				pucFileContent[fileLen] = '\0';
				sRet = pucFileContent;
				delete pucFileContent;
			}
		}
		else
		{
			sRet = _T("{\"Error\":\"OpenFileFailed\"}");
		}
	}
	else if (sAction == "IniSetKey")
	{
		CString sFile = FindKeyValue(vecActionLine, _T("File"));
		CString sSection = FindKeyValue(vecActionLine, _T("Sec"));
		CString sKey = FindKeyValue(vecActionLine, _T("Key"));
		CString sValue = FindKeyValue(vecActionLine, _T("Value"));

		if (sFile[0] != '.')
		{
			sFile = _T("./") + sFile;
		}

		if (TRUE != WritePrivateProfileString(sSection, sKey, sValue, sFile))
		{
			sRet = _T("{\"Error\":\"WriteFileFailed\"}");
		}
	}
	else if (sAction == "IniGetKey")
	{
		CString sFile = FindKeyValue(vecActionLine, _T("File"));
		CString sSection = FindKeyValue(vecActionLine, _T("Sec"));
		CString sKey = FindKeyValue(vecActionLine, _T("Key"));
		CString sValue;

		if (sFile[0] != '.')
		{
			sFile = _T("./") + sFile;
		}

		if (0 == GetPrivateProfileString(sSection, sKey, _T(""), sValue.GetBuffer(MAX_PATH), MAX_PATH, sFile))
		{
			sRet = _T("{\"Error\":\"ReadFileFailed\"}");
		}
		else
		{
			sRet.Format(_T("{\"Value\":\"%s\"}"), sValue);
		}
	}
	else if (sAction == "GetSelfPath")
	{
		CString strPath;
		DWORD dwSize=MAX_PATH;

		::GetModuleFileName(NULL,strPath.GetBuffer(MAX_PATH),dwSize); //AfxGetResourceHandle()
		strPath.ReleaseBuffer(dwSize);

		sRet = "{\"SelfPath\"=\"";
		sRet += strPath;
		sRet += "\"}";
	}
	else if (sAction == "System")
	{
		_wsystem(FindKeyValue(vecActionLine, _T("Cmd")));
	}
	else if (sAction == "WinExec")
	{
		int iMode = SW_SHOW;
		CString sHide = FindKeyValue(vecActionLine, _T("Hide"));
		if (sHide == "true")
		{
			iMode = SW_HIDE;
		}
		int len = WideCharToMultiByte(CP_ACP, 0, FindKeyValue(vecActionLine, _T("Cmd")), -1, NULL, 0, NULL, FALSE);
		if (len > 0)
		{
			char *pCmd = new char[len];
			WideCharToMultiByte(CP_ACP, 0, FindKeyValue(vecActionLine, _T("Cmd")), -1, pCmd, len, NULL, FALSE);
			WinExec(pCmd, iMode);
			delete pCmd;
		}
	}
	else if (sAction == "LoadDll")
	{
		CString sDllFile = FindKeyValue(vecActionLine, _T("File"));
		if (NULL == g_DllList.LoadDllFile(sDllFile))
		{
			sRet = _T("{\"Error\":\"LoadFileError\"}");
		}
	}
	else if (sAction == "DllCall")
	{
		CString sDllFile = FindKeyValue(vecActionLine, _T("File"));
		CString sFunc = FindKeyValue(vecActionLine, _T("Func"));
		CString sCmd = FindKeyValue(vecActionLine, _T("Cmd"));
		
		sRet = g_DllList.DllCall(sDllFile, sFunc, sCmd);
	}
	else if (sAction == "TestConnect")
	{
		INT iSocket;
		CHAR *pcDst;
		CString sDst = FindKeyValue(vecActionLine, _T("Dest"));
		CString sPort = FindKeyValue(vecActionLine, _T("Port"));

		pcDst = String2Char(sDst);

		iSocket = Socket_Create(AF_INET, SOCK_STREAM);
		if (iSocket < 0)
		{
			sRet = _T("{\"Error\":\"Can't open socket\"}");
		}
		else
		{
			if (BS_OK != Socket_Connect(iSocket, Socket_NameToIpHost(pcDst), _ttoi(sPort)))
			{
				sRet = _T("{\"Error\":\"Can't open socket\"}");
			}
		}

		delete pcDst;
	}
	else
	{
		sRet = _T("{\"Error\":\"NoSuchAction\"}");
	}

	return sRet;
}

