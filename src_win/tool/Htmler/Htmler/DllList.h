#pragma once

#include <vector>
using namespace std;

typedef VOID (*PF_DLL_CMD_FUNC)(CHAR *pcCmd /* utf8格式*/, CHAR *pcResult, INT iResultSize);

class CDllListNode
{
public:
	CString sDllFile;
	HINSTANCE hDllInst;
};

class CDllList
{
public:
	CDllList(void);
	~CDllList(void);
	HINSTANCE LoadDllFile(CString sDllFile);
	CString DllCall(CString sDllFile, CString sFunc, CString sCmd);

private:
	HINSTANCE FindDll(CString sDllFile);

private:
	vector<CDllListNode> m_vecDllList;
};



