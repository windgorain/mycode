/***=========================================================================
====                                                                     ====
====                          D C U t i l i t y                          ====
====                                                                     ====
=============================================================================
====                                                                     ====
====    File name           :  TrueColorToolBar.h                        ====
====    Project name        :  Tester                                    ====
====    Project number      :  ---                                       ====
====    Creation date       :  13/1/2003                                 ====
====    Author(s)           :  Dany Cantin                               ====
====                                                                     ====
====                  Copyright © DCUtility  2003                        ====
====                                                                     ====
=============================================================================
===========================================================================*/


#ifndef TRUECOLORTOOLBAR_H_
#define TRUECOLORTOOLBAR_H_

#if _MSC_VER > 1000
#pragma once
#endif 


#include <afxtempl.h>




class CTrueColorToolBar : public CToolBar
{

public:
	CTrueColorToolBar();


private:
	BOOL m_bDropDown;

	struct stDropDownInfo {
	public:
		UINT  uButtonID;
		UINT  uMenuID;
		CWnd* pParent;
	};
	
	CArray <stDropDownInfo, stDropDownInfo&> m_lstDropDownButton;
	

public:
	BOOL LoadTrueColorToolBar(int  nBtnWidth,
							  UINT uToolBar,
							  UINT uToolBarHot		= 0,
							  UINT uToolBarDisabled = 0);

	void AddDropDownButton(CWnd* pParent, UINT uButtonID, UINT uMenuID);

private:
	BOOL SetTrueColorToolBar(UINT uToolBarType,
		                     UINT uToolBar,
						     int  nBtnWidth);


	
	
	


public:
	virtual ~CTrueColorToolBar();

	
protected:
	
	afx_msg void OnToolbarDropDown(NMTOOLBAR* pnmh, LRESULT* plRes);
	

	DECLARE_MESSAGE_MAP()
};






#endif 
