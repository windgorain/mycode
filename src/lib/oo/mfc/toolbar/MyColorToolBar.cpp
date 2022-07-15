/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-10-30
* Description: 
* History:     
******************************************************************************/

#include "stdafx.h"

#include "oolibmfc/mycolortoolbar.h"


CMyColorToolBar::CMyColorToolBar()
{
	m_bDropDown = FALSE;
}

CMyColorToolBar::~CMyColorToolBar()
{
}


BEGIN_MESSAGE_MAP(CMyColorToolBar, CToolBar)
	ON_NOTIFY_REFLECT(TBN_DROPDOWN, OnToolbarDropDown)
END_MESSAGE_MAP()


BOOL CMyColorToolBar::LoadTrueColorToolBar
(
    int  nBtnWidth,
    UINT uToolBar,
    UINT uToolBarHot,
    UINT uToolBarDisabled
)
{
	if (!SetTrueColorToolBar(TB_SETIMAGELIST, uToolBar, nBtnWidth))
		return FALSE;
	
	if (uToolBarHot) {
		if (!SetTrueColorToolBar(TB_SETHOTIMAGELIST, uToolBarHot, nBtnWidth))
			return FALSE;
	}

	if (uToolBarDisabled) {
		if (!SetTrueColorToolBar(TB_SETDISABLEDIMAGELIST, uToolBarDisabled, nBtnWidth))
			return FALSE;
	}

	return TRUE;
}

BOOL CMyColorToolBar::SetTrueColorToolBar
(
    UINT uToolBarType, 
    UINT uToolBar,
    int  nBtnWidth
)
{
	CImageList	cImageList;
	CBitmap		cBitmap;
	BITMAP		bmBitmap;
	
	if ((!cBitmap.Attach(LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(uToolBar),
				  IMAGE_BITMAP, 0, 0,
				  LR_DEFAULTSIZE|LR_CREATEDIBSECTION)))
          || (!cBitmap.GetBitmap(&bmBitmap)))
	{
		return FALSE;
	}

	CSize		cSize(bmBitmap.bmWidth, bmBitmap.bmHeight); 
	int			nNbBtn	= cSize.cx/nBtnWidth;
	RGBTRIPLE*	rgb		= (RGBTRIPLE*)(bmBitmap.bmBits);
	COLORREF	rgbMask	= RGB(rgb[0].rgbtRed, rgb[0].rgbtGreen, rgb[0].rgbtBlue);
	
	if (!cImageList.Create(nBtnWidth, cSize.cy, ILC_COLOR24|ILC_MASK, nNbBtn, 0))
		return FALSE;
	
	if (cImageList.Add(&cBitmap, rgbMask) == -1)
		return FALSE;

	SendMessage(uToolBarType, 0, (LPARAM)cImageList.m_hImageList);
	cImageList.Detach(); 
	cBitmap.Detach();
	
	return TRUE;
}


void CMyColorToolBar::AddDropDownButton(CWnd* pParent, UINT uButtonID, UINT uMenuID)
{
	if (!m_bDropDown)
    {
		GetToolBarCtrl().SendMessage(TB_SETEXTENDEDSTYLE, 0, (LPARAM)TBSTYLE_EX_DRAWDDARROWS);
		m_bDropDown = TRUE;
	}

	SetButtonStyle(CommandToIndex(uButtonID), TBSTYLE_DROPDOWN);

	stDropDownInfo DropDownInfo;
	DropDownInfo.pParent	= pParent;
	DropDownInfo.uButtonID	= uButtonID;
	DropDownInfo.uMenuID	= uMenuID;
	m_lstDropDownButton.Add(DropDownInfo);
}

void CMyColorToolBar::OnToolbarDropDown(NMTOOLBAR* pnmtb, LRESULT *plr)
{
	for (int i = 0; i < m_lstDropDownButton.GetSize(); i++) {
		
		stDropDownInfo DropDownInfo = m_lstDropDownButton.GetAt(i);

		if (DropDownInfo.uButtonID == UINT(pnmtb->iItem)) {

			CMenu menu;
			menu.LoadMenu(DropDownInfo.uMenuID);
			CMenu* pPopup = menu.GetSubMenu(0);
			
			CRect rc;
			SendMessage(TB_GETRECT, (WPARAM)pnmtb->iItem, (LPARAM)&rc);
			ClientToScreen(&rc);
			
			pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL,
				                   rc.left, rc.bottom, DropDownInfo.pParent, &rc);
			break;
		}
	}
}

