
// HtmlerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Htmler.h"
#include "HtmlerDlg.h"
#include "DllList.h"
#include "afxdialogex.h"
#include <gdiplus.h>
#pragma comment(lib,"GdiPlus.lib")

#include "bs.h"
#include "utl/tray_utl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_TRAY_NOTIFY			WM_USER+101

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CHtmlerDlg 对话框

BEGIN_DHTML_EVENT_MAP(CHtmlerDlg)
END_DHTML_EVENT_MAP()


CHtmlerDlg::CHtmlerDlg(CWnd* pParent /*=NULL*/)
	: CDHtmlDialog(CHtmlerDlg::IDD, CHtmlerDlg::IDH, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	EnableAutomation();
	m_sIndex = "index.htm";
	m_sDoAction = "";
	m_bIsModless = FALSE;
}

void CHtmlerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDHtmlDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CHtmlerDlg, CDHtmlDialog)
	ON_WM_SYSCOMMAND()
	ON_MESSAGE(WM_TRAY_NOTIFY, &CHtmlerDlg::my_OnTrayNotify)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


BEGIN_DISPATCH_MAP(CHtmlerDlg, CDHtmlDialog)
DISP_FUNCTION(CHtmlerDlg,"HtmlerCmd", HtmlerCmd, VT_BSTR, VTS_VARIANT)
END_DISPATCH_MAP() 


// CHtmlerDlg 消息处理程序

BOOL CHtmlerDlg::OnInitDialog()
{
	CDHtmlDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	my_DoInitAction();
	SetExternalDispatch(GetIDispatch(TRUE)); 
	my_GotoUrl(m_sIndex);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CHtmlerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDHtmlDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CHtmlerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDHtmlDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CHtmlerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CHtmlerDlg::OnCancel()
{
	return;
}

void CHtmlerDlg::OnOK()
{
	return;
}

void CHtmlerDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDHtmlDialog::OnCancel();
}

void CHtmlerDlg::PostNcDestroy()
{
	CDHtmlDialog::PostNcDestroy();
	if (m_bIsModless)
	{
		delete this;
	}
}

void CHtmlerDlg::OnBeforeNavigate(
   LPDISPATCH pDisp,
   LPCTSTR szUrl 
)
{
	CDHtmlDialog::OnBeforeNavigate(pDisp, szUrl);
}

STDMETHODIMP CHtmlerDlg::GetHostInfo(DOCHOSTUIINFO* pInfo)
{
	pInfo->dwFlags = DOCHOSTUIFLAG_THEME;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CHtmlerDlg::ShowContextMenu(DWORD dwID,  POINT *ppt,  IUnknown *pcmdtReserved,  IDispatch *pdispReserved)
{
	return S_OK;
}

void CHtmlerDlg::my_OnCancel()
{
	if (m_bIsModless)
	{
		DestroyWindow();
	}
	else
	{
		CDHtmlDialog::OnCancel();
	}
}

void CHtmlerDlg::my_OnOK()
{
	if (m_bIsModless)
	{
		UpdateData(TRUE);
		DestroyWindow();
	}
	else
	{
		CDHtmlDialog::OnOK();
	}
}

void CHtmlerDlg::my_SetIndex(CString sIndex)
{
	m_sIndex = sIndex;
}

void CHtmlerDlg::my_GotoUrl(CString sIndex)
{
    CString strPath,str;

	if (sIndex == "")
	{
		return;
	}

	sIndex = sIndex.Trim();

	if ((sIndex.Mid(0,7) == "http://") || (sIndex.Mid(0,8) == "https://"))
	{
		Navigate(sIndex);
		return;
	}

    DWORD dwSize=MAX_PATH;

    ::GetModuleFileName(NULL,strPath.GetBuffer(MAX_PATH),dwSize); //AfxGetResourceHandle()
    strPath.ReleaseBuffer(dwSize);

    str=strPath.Left(strPath.ReverseFind('\\')+1);

    CString strUrl;
    strUrl = str + _T("htm\\") + sIndex;
    Navigate(_T("file:///")+strUrl);
}

CString CHtmlerDlg::my_DoActionOne(CString sActionLine)
{
	vector<CKeyValue> vecActionLine;
	CString sRet = _T("");
	CString sAction;

	StringParseParams(sActionLine, ',', '=', vecActionLine);

	sAction = FindKeyValue(vecActionLine, _T("Action"));
	
	if (sAction == "SetTitle")
	{
		this->SetWindowText(FindKeyValue(vecActionLine, _T("Title")));
	}
    else if (sAction == "CallJsFunction")
	{
		CString sFunction = FindKeyValue(vecActionLine, _T("Function"));
		CString sParam = FindKeyValue(vecActionLine, _T("Param"));
		my_CallJsFunc(sFunction, sParam);
	}
	else if (sAction == "SetIcon")
	{
		CImage img;

		img.Load( FindKeyValue(vecActionLine, _T("Icon")) );
		if(!img.IsNull() )
		{
			HBITMAP hBitmap = img.Detach();

			Gdiplus::Bitmap* pTmpBitmap=Gdiplus::Bitmap::FromHBITMAP(hBitmap,NULL);
			HICON hIcon=NULL;
			pTmpBitmap->GetHICON(&hIcon);
			delete pTmpBitmap;
			SetIcon(hIcon,FALSE); //设置小图标
			AfxGetMainWnd()->SetIcon(hIcon,TRUE); //设置大图标 
			m_hIcon = hIcon;
		}
	}
	else if (sAction == "AddMenuItem")
	{
		sRet = my_AddMenuItem(vecActionLine);
	}
	else if (sAction == "GotoUrl")
	{
		CString sUrl = FindKeyValue(vecActionLine, _T("URL"));
		my_GotoUrl(sUrl);
	}
	else if (sAction == "GetSize")
	{
		CRect rect;
		GetWindowRect(rect);
		sRet.Format(_T("{\"W\":%d,\"H\":%d}"), rect.Width(), rect.Height());
	}
	else if (sAction == "SetSize")
	{
		CRect rect;
		GetWindowRect(rect);
		CPoint stPoint = rect.TopLeft();
		CString sWidth = FindKeyValue(vecActionLine, _T("W"));
		CString sHeight = FindKeyValue(vecActionLine, _T("H"));
		if (sWidth=="")
		{
			sWidth.Format(_T("%d"), rect.Width());
		}
		if (sHeight == "")
		{
			sHeight.Format(_T("%d"), rect.Height());
		}

		int w = _ttoi(sWidth);
		int h = _ttoi(sHeight);

		::SetWindowPos(this->m_hWnd, HWND_BOTTOM, stPoint.x, stPoint.y, w, h, SWP_NOZORDER);
	}
	else if (sAction == "SetOffset")
	{
		CRect rect;
		GetWindowRect(rect);
		CPoint stPoint = rect.TopLeft();
		CPoint stPoint2 = rect.BottomRight();
		CString sXOffset = FindKeyValue(vecActionLine, _T("X"));
		CString sYOffset = FindKeyValue(vecActionLine, _T("Y"));

		if (sXOffset=="")
		{
			sXOffset = "0";
		}
		if (sYOffset == "")
		{
			sYOffset = "0";
		}

		int x = _ttoi(sXOffset);
		int y = _ttoi(sYOffset);

		::SetWindowPos(this->m_hWnd, HWND_BOTTOM,
			stPoint.x + x, stPoint.y + y,
			stPoint2.x, stPoint2.y, SWP_NOZORDER);
	}
	else if (sAction == "SetPosition")
	{
		CRect rect;
		GetWindowRect(rect);
		CString sX = FindKeyValue(vecActionLine, _T("X"));
		CString sY = FindKeyValue(vecActionLine, _T("Y"));

		if (sX=="")
		{
			sX = "0";
		}
		if (sY == "")
		{
			sY = "0";
		}

		int x = _ttoi(sX);
		int y = _ttoi(sY);

		::SetWindowPos(this->m_hWnd, HWND_BOTTOM,
			x, y, rect.Width(), rect.Height(), SWP_NOZORDER);
	}
	else if (sAction == "SetMaximized")
	{
		ShowWindow(SW_SHOWMAXIMIZED);
		UpdateWindow();
	}
	else if (sAction == "SetMinimize")
	{
		ShowWindow(SW_MINIMIZE);
		UpdateWindow();
	}
	else if (sAction == "SetStyle")
	{
		CString sMax = FindKeyValue(vecActionLine, _T("Max"));
		CString sMin = FindKeyValue(vecActionLine, _T("Min"));
		CString sResize = FindKeyValue(vecActionLine, _T("Resize"));

		LONG lFlag = GetWindowLong(this->m_hWnd, GWL_STYLE);

		if (sMax == "true")
		{
			lFlag |= WS_MAXIMIZEBOX;
		}
		else if (sMax == "false")
		{
			lFlag &= ~WS_MAXIMIZEBOX;
		}

		if (sMin == "true")
		{
			lFlag |= WS_MINIMIZEBOX;
		}
		else if (sMin == "false")
		{
			lFlag &= ~WS_MINIMIZEBOX ;
		}

		if (sResize == "true")
		{
			lFlag |= WS_SIZEBOX ;
		}
		else if (sResize == "false")
		{
			lFlag &= ~WS_SIZEBOX ;
		}

		SetWindowLong(this->m_hWnd, GWL_STYLE, lFlag);
	}
	else if (sAction == "HtmlerOK")
	{
		if (m_bIsModless)
		{
			PostMessage(WM_CLOSE);
		}
		else
		{
			my_OnOK();
		}
	}
	else if (sAction == "HtmlerCancel")
	{
		if (m_bIsModless)
		{
			PostMessage(WM_CLOSE);
		}
		else
		{
			my_OnCancel();
		}
	}
	else if (sAction == "Hide")
	{
		ShowWindow(SW_HIDE);
	}
	else if (sAction == "Show")
	{
		ShowWindow(SW_SHOW);
	}
	else
	{
		sRet = HtmlerLib_DoActionOne(vecActionLine);
	}

	return sRet;
}

CString CHtmlerDlg::my_DoActionList(CString sActionList)
{
	CStringArray astActionArray;

	StringSplit(sActionList, ';', astActionArray);

	if (astActionArray.GetSize() == 0)
	{
		return  _T("{\"Error\":\"EmptyActionList\"}");
	}

	CString sRet;

	for (int i =0; i< astActionArray.GetSize(); i++)
	{
		sRet = my_DoActionOne(astActionArray[i]);
	}

	return sRet;
}

BSTR CHtmlerDlg::HtmlerCmd(VARIANT& vActionList)
{
	CString sActionList;
	CString sParams;
	CString sFlags;

	sActionList.Format(_T("%s"), vActionList.bstrVal);

	CString sRet = my_DoActionList(sActionList);

	BSTR pStr = SysAllocString(sRet);

	return pStr;
}

BOOL CHtmlerDlg::CanAccessExternal ()
{
	return TRUE;
} 

void CHtmlerDlg::my_SetModless()
{
	m_bIsModless = TRUE;
}

void CHtmlerDlg::my_SetInitDoAction(CString sDoAction)
{
	m_sDoAction = sDoAction;
}

void CHtmlerDlg::my_DoInitAction()
{
	if (m_sDoAction == "")
	{
		return;
	}

	my_DoActionList(m_sDoAction);
}

afx_msg LRESULT CHtmlerDlg::my_OnTrayNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam != 1)
	{
		return 0;
	}

	if (lParam == WM_LBUTTONDOWN)
	{
		ShowWindow(SW_SHOW);
	}

	return 0;
}


BOOL CHtmlerDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	// TODO: 在此添加专用代码和/或调用基类
	CString sActions = HtmlerLib_GetMenuAction(LOWORD(wParam));
	if (sActions != "")
	{
		my_DoActionList(sActions);
	}

	return CDHtmlDialog::OnCommand(wParam, lParam);
}

CMenu * CHtmlerDlg::my_FindMenu(IN CMenu *pParentMenu, IN CString sMenu)
{
	CMenu *pMenuFound = NULL;

	int iCount = pParentMenu->GetMenuItemCount();

	for (int i=0; i<iCount; i++)
	{
		CString sMenuString;
		
		if (0 == pParentMenu->GetMenuStringW(i, sMenuString, MF_BYPOSITION))
		{
			continue;
		}

		CMenu *pMenu = pParentMenu->GetSubMenu(i);

		if ((NULL != pMenu) && (sMenuString == sMenu))
		{
			pMenuFound = pMenu;
			break;
		}
	}

	return pMenuFound;
}

int CHtmlerDlg::my_FindMenuItem(IN CMenu *pParentMenu, IN CString sItem)
{
	int iIndex = -1;

	int iCount = pParentMenu->GetMenuItemCount();

	for (int i=0; i<iCount; i++)
	{
		CString sMenuString;
		
		if (0 == pParentMenu->GetMenuStringW(i, sMenuString, MF_BYPOSITION))
		{
			continue;
		}

		if (sMenuString == sItem)
		{
			iIndex = i;
			break;
		}
	}

	return iIndex;
}

/* 添加一个menu,并返回Menu指针 */
CMenu * CHtmlerDlg::my_AddPopMenu(IN CMenu *pstParentMenu, IN CString sMenu)
{
	CMenu menu;
	menu.CreatePopupMenu();
	pstParentMenu->AppendMenu(MF_POPUP, (UINT)menu.m_hMenu, sMenu);
	menu.Detach();

	return my_FindMenu(pstParentMenu, sMenu);
}

CString CHtmlerDlg::my_AddMenuItem(IN vector<CKeyValue> &vecActionLine)
{
	CString sRet = _T("");
	CMenu *pMenuLast = NULL;

	CString sMenu = FindKeyValue(vecActionLine, _T("Menu"));
	CString sItem = FindKeyValue(vecActionLine, _T("Item"));
	CString sDoActions = FindKeyValue(vecActionLine, _T("DoAction"));
	if ((sMenu == "") || (sItem == "") || (sDoActions == ""))
	{
		return _T("{\"Error\":\"BadParam\"}");
	}

	CStringArray astMenu;
	StringSplit(sMenu, '.', astMenu);
	if (astMenu.GetSize() == 0)
	{
		return _T("{\"Error\":\"BadParam\"}");
	}

	pMenuLast = my_AddMenus(astMenu);

	if (NULL == pMenuLast)
	{
		return _T("{\"Error\":\"CanNotCreateMenu\"}");
	}

	if (my_FindMenuItem(pMenuLast, sItem) >= 0)
	{
		return _T("{\"Error\":\"Exist\"}");
	}

	UINT uiMenuItemID = HtmlerLib_AddMenuAction(sDoActions);

	pMenuLast->AppendMenu(MF_STRING|MF_ENABLED, uiMenuItemID, sItem);

	DrawMenuBar();

	return sRet;
}

/* 按照路径添加Menu，返回最后一级Menu的指针 */
CMenu * CHtmlerDlg::my_AddMenus(IN CStringArray &astMenu)
{
	CMenu *pParentMenu;
	CMenu *pstLastMenu;
	CMenu *pMainMenu = GetMenu();
	if (NULL == pMainMenu)
	{
		CMenu menuMain;
		menuMain.CreateMenu();
		SetMenu(&menuMain);
		menuMain.Detach();
		pMainMenu = GetMenu();
		if (pMainMenu == NULL)
		{
			return NULL;
		}
	}

	pParentMenu = pMainMenu;
	for (int i=0; i<astMenu.GetSize(); i++)
	{
		CString sMenu = astMenu[i];
		pstLastMenu = my_FindMenu(pParentMenu, sMenu);
		if (pstLastMenu == NULL)
		{
			pstLastMenu = my_AddPopMenu(pParentMenu, sMenu);
		}
		if (pstLastMenu == NULL)
		{
			return NULL;
		}
		pParentMenu = pstLastMenu;
	}

	return pstLastMenu;
}

void CHtmlerDlg::my_CallJsFunc(CString sFunctionName, CString sParam)
{
	IHTMLDocument2 *pDoc;
	HRESULT hr;
	IDispatch *pDispScript;
	DISPID dispid;
	CComVariant *pParams;
	
	hr = GetDHtmlDocument(&pDoc);

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

void CHtmlerDlg::my_CreateModelessHtmler(vector<CKeyValue> &vecActionLine)
{
	CHtmlerDlg *pDlgTmp = new CHtmlerDlg();
	pDlgTmp->my_SetInitDoAction(FindKeyValue(vecActionLine, _T("DoAction")));
	pDlgTmp->my_SetModless();
	pDlgTmp->my_SetIndex(FindKeyValue(vecActionLine, _T("URL")));
	pDlgTmp->Create(IDD_HTMLER_DIALOG,GetDesktopWindow());
	pDlgTmp->ShowWindow(SW_SHOW);
}

void CHtmlerDlg::my_CreateTray(vector<CKeyValue> &vecActionLine)
{
	HANDLE hTray = HtmlerLib_GetTray();
	if (hTray == NULL)
	{
		CString sTip = FindKeyValue(vecActionLine, _T("Tip"));
		hTray = TRAY_Create(m_hWnd, WM_TRAY_NOTIFY, 1);
		TRAY_SetIcon(hTray, m_hIcon, Unicode2Ansi(sTip.GetBuffer()));
		HtmlerLib_SetTray(hTray);
	}
}
