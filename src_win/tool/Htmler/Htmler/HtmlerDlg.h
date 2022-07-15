
// HtmlerDlg.h : 头文件
//

#pragma once

#include "HtmlerLib.h"


// CHtmlerDlg 对话框
class CHtmlerDlg : public CDHtmlDialog
{
// 构造
public:
	CHtmlerDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_HTMLER_DIALOG, IDH = 0 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public:
	void OnCancel();
	void OnOK();
	afx_msg void OnClose();
	void PostNcDestroy();
	STDMETHODIMP GetHostInfo(DOCHOSTUIINFO* pInfo);
	BSTR HtmlerCmd(VARIANT& vActionList);
	BOOL CanAccessExternal();
	void OnBeforeNavigate(LPDISPATCH pDisp, LPCTSTR szUrl);
    virtual HRESULT STDMETHODCALLTYPE ShowContextMenu(DWORD dwID,POINT *ppt,IUnknown *pcmdtReserved,IDispatch *pdispReserved);

private:
	void my_DoInitAction();
    afx_msg LRESULT my_OnTrayNotify(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	DECLARE_DHTML_EVENT_MAP()
	DECLARE_DISPATCH_MAP() 

private:
	CString m_sIndex;
	BOOL m_bIsModless;
	CString m_sDoAction; /* 初始化动作命令 */

public:
	void my_OnCancel();
	void my_OnOK();
	void my_GotoUrl(CString pszUrl);
	CString my_DoActionOne(CString sActionList);  /* 执行单个动作 */
	CString my_DoActionList(CString sActionList);  /* 执行多个动作 */
	void my_SetIndex(CString sIndex);
	void my_SetModless();
	void my_SetInitDoAction(CString sDoAction);
	void my_CallJsFunc(CString sFunctionName, CString sParam);
	CMenu * my_AddPopMenu(IN CMenu *pstParentMenu, IN CString sMenu);
	CString my_AddMenuItem(IN vector<CKeyValue> &vecActionLine);
	CMenu * my_FindMenu(IN CMenu *pParentMenu, IN CString sMenu);
    int my_FindMenuItem(IN CMenu *pParentMenu, IN CString sItem);
	CMenu * my_AddMenus(IN CStringArray &astMenu);
    void my_CreateModelessHtmler(vector<CKeyValue> &vecActionLine);
    void my_CreateTray(vector<CKeyValue> &vecActionLine);
};


