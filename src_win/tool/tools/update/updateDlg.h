
// updateDlg.h : 头文件
//

#pragma once


// CupdateDlg 对话框
class CupdateDlg : public CDialogEx
{

public:
	CupdateDlg(CWnd* pParent = NULL);	


	enum { IDD = IDD_UPDATE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	



protected:
	HICON m_hIcon;

	
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
private:
	void update_init(void);
	void update_start(char *pcVerUrl);
public:
	void update_UpdateCallBack(void);
};
