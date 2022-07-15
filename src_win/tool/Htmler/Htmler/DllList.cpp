#include "stdafx.h"
#include "DllList.h"

CDllList::CDllList(void)
{
}

CDllList::~CDllList(void)
{
	m_vecDllList.clear();
}

HINSTANCE CDllList::LoadDllFile(CString sDllFile)
{
	HINSTANCE hDllInst;

	hDllInst = FindDll(sDllFile);
	if (NULL == hDllInst)
	{
		hDllInst = LoadLibrary(sDllFile);
		if (NULL != hDllInst)
		{
			CDllListNode dllNode;
			dllNode.sDllFile = sDllFile;
			dllNode.hDllInst = hDllInst;
			m_vecDllList.push_back(dllNode);
		}
	}

	return hDllInst;
}

HINSTANCE CDllList::FindDll(CString sDllFile)
{
	for (unsigned i=0; i<m_vecDllList.size(); i++)
	{
		if (m_vecDllList[i].sDllFile == sDllFile)
		{
			return m_vecDllList[i].hDllInst;
		}
	}

	return NULL;
}

CString CDllList::DllCall(CString sDllFile, CString sFunc, CString sCmd)
{
	int len;
	HINSTANCE hInst;
	PF_DLL_CMD_FUNC pfFunc;
	CString sRet = _T("{}");

	hInst = LoadDllFile(sDllFile);
	if (NULL == hInst)
	{
		return _T("{\"Error\":\"LoadFileError\"}");
	}

	len = WideCharToMultiByte(CP_UTF8, 0, sFunc, -1, NULL, 0, NULL, FALSE);
	if (len <= 0)
	{
		return _T("{\"Error\":\"CanNotLoadFunc\"}");
	}
	
	char *pcFunc = new char[len];
	WideCharToMultiByte(CP_UTF8, 0, sFunc, -1, pcFunc, len, NULL, FALSE);
	pfFunc = (PF_DLL_CMD_FUNC)GetProcAddress(hInst, pcFunc);
	delete pcFunc;
	if (NULL == pfFunc)
	{
		return _T("{\"Error\":\"LoadCmdFunctionError\"}");
	}

	len = WideCharToMultiByte(CP_UTF8, 0, sCmd, -1, NULL, 0, NULL, FALSE);
	if (len > 0)
	{
		char *pCmd = new char[len];
		char *pcResult = new char[1024];
		pcResult[0] = '\0';
		WideCharToMultiByte(CP_UTF8, 0, sCmd, -1, pCmd, len, NULL, FALSE);
		pfFunc(pCmd, pcResult, 1024);
		delete pCmd;

		len = MultiByteToWideChar(CP_UTF8, 0, pcResult, -1, NULL, 0);
		if (len > 0)
		{
			wchar_t *pWResult = new wchar_t[len];
			MultiByteToWideChar(CP_UTF8, 0, pcResult, -1, pWResult, len);
			sRet = pWResult;
			delete pWResult;
		}

		delete pcResult;
	}

	return sRet;
}