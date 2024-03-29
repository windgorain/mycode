/************************************************************************/
/* WMI 基础类库封装							*/



#ifndef __WMI_H__
#define __WMI_H__
#include "windows.h"
#include "Wbemcli.h"

class WMILib{
public:
	WMILib();
	~WMILib();
public:
	HRESULT m_hResult;
	bool m_bIsInit;
	IWbemLocator* m_pLoc;
	IWbemServices* m_pSvc;
	IWbemClassObject* m_pClass;
	IWbemClassObject* m_pInParamsDefinition;
	IWbemClassObject* m_pClassInstance;
	IWbemClassObject* m_pOutParams;
public:
	
	bool Init(LPCTSTR Svr);
	bool ModifyDns(LPCTSTR lpszDns1, LPCTSTR lpszDns2);
	bool ModifyIP(LPCTSTR lpszIP, LPCTSTR lpszNetmask, LPCTSTR lpszGateway);
	bool ModifyNetwork(LPCTSTR lpszIP, LPCTSTR lpszNetmask, LPCTSTR lpszGateway, LPCTSTR lpszDns1, LPCTSTR lpszDns2);
	HRESULT GetResult();
	void PrintWMIError(HRESULT hr);
	void  CreateOneElementBstrArray(VARIANT*  v,  LPCWSTR  s);
};

#endif