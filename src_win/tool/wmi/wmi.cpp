
#include "wmi.h"
#include "stdio.h"
#include "stdlib.h"
#include "comutil.h"
#include "atlbase.h"
#pragma comment(lib, "wbemuuid.lib")//wmi
#pragma comment(lib, "comsuppw.lib ")

WMILib::WMILib()
{
	m_pLoc = NULL;
	m_pSvc = NULL;
	m_pClass = NULL;
	m_pClassInstance = NULL;
	m_pInParamsDefinition = NULL;
	m_pOutParams = NULL;
	m_bIsInit = false;
}

WMILib::~WMILib()
{
	if (NULL != m_pLoc)
		m_pLoc->Release();
	if (NULL != m_pSvc)
		m_pSvc->Release();
	if (NULL != m_pClass)
		m_pClass->Release();
	if (NULL != m_pClassInstance)
		m_pClassInstance->Release();
	if (NULL != m_pInParamsDefinition)
		m_pInParamsDefinition->Release();
	if (m_bIsInit)
		CoUninitialize();
}

HRESULT WMILib::GetResult()
{
	return m_hResult;
}

bool WMILib::Init(LPCTSTR Svr = "ROOT\\CIMV2")
{
	if (m_bIsInit)
	{
		return true;
	}
	// Step 1: --------------------------------------------------
	// Initialize COM. ------------------------------------------
	m_hResult =  CoInitializeEx(0, COINIT_MULTITHREADED); 
	if (FAILED(m_hResult))
	{
		printf("Failed to initialize COM library. Error code = 0x%d", m_hResult);
		return false;                  // Program has failed.
	}

	// Step 2: --------------------------------------------------
	// Set general COM security levels --------------------------

	m_hResult =  CoInitializeSecurity(
		NULL, 
		-1,                          // COM negotiates service
		NULL,                        // Authentication services
		NULL,                        // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
		RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
		NULL,                        // Authentication info
		EOAC_NONE,                   // Additional capabilities 
		NULL                         // Reserved
		);

	if (FAILED(m_hResult))
	{
		printf("Failed to initialize security. Error code = 0x%d", m_hResult);
		CoUninitialize();
		return false;                      // Program has failed.
	}

	// Step 3: ---------------------------------------------------
	// Obtain the initial locator to WMI -------------------------


	m_hResult = CoCreateInstance(
		CLSID_WbemLocator,
		0, 
		CLSCTX_INPROC_SERVER, 
		IID_IWbemLocator, (LPVOID *) &m_pLoc);

	if (FAILED(m_hResult))
	{
		printf("Failed to create IWbemLocator object.");
		CoUninitialize();
		return false;                 // Program has failed.
	}

	// Step 4: ---------------------------------------------------
	// Connect to WMI through the IWbemLocator::ConnectServer method

	// Connect to the local root\cimv2 namespace
	// and obtain pointer pSvc to make IWbemServices calls.
	m_hResult = m_pLoc->ConnectServer(
		_bstr_t(CComBSTR(Svr)), 
		NULL,
		NULL, 
		0, 
		NULL, 
		0, 
		0, 
		&m_pSvc
		);

	if (FAILED(m_hResult))
	{
		printf("Could not connect. Error code=0x%d", m_hResult);
		m_pLoc->Release();
		CoUninitialize();
		return false;                // Program has failed.
	}

	//cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


	// Step 5: --------------------------------------------------
	// Set security levels for the proxy ------------------------

	m_hResult = CoSetProxyBlanket(
		m_pSvc,                        // Indicates the proxy to set
		RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx 
		RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx 
		NULL,                        // Server principal name 
		RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		NULL,                        // client identity
		EOAC_NONE                    // proxy capabilities 
		);

	if (FAILED(m_hResult))
	{
		printf("Could not set proxy blanket. Error code=0x%d", m_hResult);
		m_pSvc->Release();
		m_pLoc->Release();
		CoUninitialize();
		return false;               // Program has failed.
	}
	return m_bIsInit = true;
}

bool WMILib::ModifyDns(LPCTSTR lpszDns1, LPCTSTR lpszDns2)
{
	if(false == Init())
		return false;

	BSTR InstancePath = SysAllocString(L"Win32_NetworkAdapterConfiguration.index='7'"); //indexΪ������
	BSTR ClassPath = SysAllocString(L"Win32_NetworkAdapterConfiguration");
	BSTR MethodName1 = SysAllocString(L"SetDNSServerSearchOrder");
	BSTR MethodName2 = SysAllocString(L"EnableDHCP");
	LPCWSTR MethodName1ArgName = L"DNSServerSearchOrder";
	BSTR dns1 = CComBSTR(lpszDns1);
	BSTR dns2 = CComBSTR(lpszDns2);

	long DnsIndex1[] = {0};
	long DnsIndex2[] = {1};

	SAFEARRAY *ip_list = SafeArrayCreateVector(VT_BSTR, 0, 2);
	SafeArrayPutElement(ip_list, DnsIndex1, dns1);
	SafeArrayPutElement(ip_list, DnsIndex2, dns2);
	VARIANT dns;
	dns.vt = VT_ARRAY | VT_BSTR;
	dns.parray = ip_list;

	m_hResult = m_pSvc->GetObject(ClassPath, 0, NULL, &m_pClass, NULL);
	if (!SUCCEEDED(m_hResult)) {
		PrintWMIError(m_hResult);
		return false;
	}
	if (SUCCEEDED(m_hResult))
		m_hResult = m_pClass->GetMethod(MethodName1, 0, &m_pInParamsDefinition, NULL);
	if (!SUCCEEDED(m_hResult)) {
		PrintWMIError(m_hResult);
		return false;
	}
	if (SUCCEEDED(m_hResult))
		m_hResult = m_pInParamsDefinition->SpawnInstance(0, &m_pClassInstance);
	if (!SUCCEEDED(m_hResult)) {
		PrintWMIError(m_hResult);
		return false;
	}
	if (SUCCEEDED(m_hResult)) {
		m_hResult = m_pClassInstance->Put(MethodName1ArgName, 0, &dns, 0);
	}


	if (!SUCCEEDED(m_hResult)) {
		PrintWMIError(m_hResult);
		return false;
	}
	if (SUCCEEDED(m_hResult))
		m_hResult = m_pSvc->ExecMethod(InstancePath, MethodName1, 0, NULL,
		m_pClassInstance, &m_pOutParams, NULL);
	if (!SUCCEEDED(m_hResult)) {
		PrintWMIError(m_hResult);
		return false;
	}
	/*if (SUCCEEDED(m_hResult))
		m_hResult = m_pSvc->ExecMethod(InstancePath, MethodName2, 0, NULL, NULL,
		&m_pOutParams, NULL);
	if (!SUCCEEDED(m_hResult)) {
		PrintWMIError(m_hResult);
		return false;
	}*/
	SysFreeString(InstancePath);
	SysFreeString(ClassPath);
	SysFreeString(MethodName1);
	SysFreeString(MethodName2);
	return true;
}

bool WMILib::ModifyIP(LPCTSTR lpszIP, LPCTSTR lpszNetmask, LPCTSTR lpszGateway)
{
	if(false ==Init())
		return false;
	BSTR  InstancePath  =  SysAllocString(L"Win32_NetworkAdapterConfiguration.index=7");      
	BSTR  ClassPath  =  SysAllocString(L"Win32_NetworkAdapterConfiguration");    
	//modify ip address and netmask 
	BSTR  MethodName  =  SysAllocString(L"EnableStatic");
	LPCWSTR  Arg1Name  =  L"IPAddress";    
	VARIANT  var1;    
	LPCWSTR  Arg2Name  =  L"SubnetMask";    
	VARIANT  var2;
	CreateOneElementBstrArray(&var1,  CComBSTR(lpszIP));    
	CreateOneElementBstrArray(&var2,  CComBSTR(lpszNetmask));
	m_hResult = m_pSvc->GetObject(ClassPath, 0, NULL, &m_pClass, NULL);
	if (!SUCCEEDED(m_hResult)) {
		PrintWMIError(m_hResult);
		return false;
	}
	if (SUCCEEDED(m_hResult))
		m_hResult = m_pClass->GetMethod(MethodName, 0, &m_pInParamsDefinition, NULL);
	if (!SUCCEEDED(m_hResult)) {
		PrintWMIError(m_hResult);
		return false;
	}
	if (SUCCEEDED(m_hResult))
		m_hResult = m_pInParamsDefinition->SpawnInstance(0, &m_pClassInstance);
	if (!SUCCEEDED(m_hResult)) {
		PrintWMIError(m_hResult);
		return false;
	}
	if (SUCCEEDED(m_hResult)) {
		m_hResult = m_pClassInstance->Put(Arg1Name, 0, &var1, 0);
	}
	if (SUCCEEDED(m_hResult)) {
		m_hResult = m_pClassInstance->Put(Arg2Name, 0, &var2, 0);
	}
	if (SUCCEEDED(m_hResult))
		m_hResult = m_pSvc->ExecMethod(InstancePath, MethodName, 0, NULL,
		m_pClassInstance, &m_pOutParams, NULL);
	if (!SUCCEEDED(m_hResult)) {
		PrintWMIError(m_hResult);
		return false;
	}
	SysFreeString(ClassPath);
	SysFreeString(MethodName);
	// modify gateway
	BSTR  MethodName2  =  SysAllocString(L"SetGateways");
	LPCWSTR  Arg1Name2  =  L"DefaultIPGateway";    
	VARIANT  var3;    
	LPCWSTR  Arg2Name2  =  L"GatewayCostMetric";    
	VARIANT  var4;
	CreateOneElementBstrArray(&var3,  CComBSTR(lpszGateway));    
	CreateOneElementBstrArray(&var4,  CComBSTR(1));
	m_hResult = m_pSvc->GetObject(ClassPath, 0, NULL, &m_pClass, NULL);
	if (!SUCCEEDED(m_hResult)) {
		PrintWMIError(m_hResult);
		return false;
	}
	if (SUCCEEDED(m_hResult))
		m_hResult = m_pClass->GetMethod(MethodName2, 0, &m_pInParamsDefinition, NULL);
	if (!SUCCEEDED(m_hResult)) {
		PrintWMIError(m_hResult);
		return false;
	}
	if (SUCCEEDED(m_hResult))
		m_hResult = m_pInParamsDefinition->SpawnInstance(0, &m_pClassInstance);
	if (!SUCCEEDED(m_hResult)) {
		PrintWMIError(m_hResult);
		return false;
	}
	if (SUCCEEDED(m_hResult)) {
		m_hResult = m_pClassInstance->Put(Arg1Name2, 0, &var3, 0);
	}
	if (SUCCEEDED(m_hResult)) {
		m_hResult = m_pClassInstance->Put(Arg2Name2, 0, &var4, 0);
	}
	if (SUCCEEDED(m_hResult))
		m_hResult = m_pSvc->ExecMethod(InstancePath, MethodName2, 0, NULL,
		m_pClassInstance, &m_pOutParams, NULL);
	if (!SUCCEEDED(m_hResult)) {
		PrintWMIError(m_hResult);
		return false;
	}
	SysFreeString(InstancePath);
	SysFreeString(ClassPath);
	SysFreeString(MethodName2);
	return true;
}

bool WMILib::ModifyNetwork(LPCTSTR lpszIP, LPCTSTR lpszNetmask, LPCTSTR lpszGateway, LPCTSTR lpszDns1, LPCTSTR lpszDns2)
{
	return ModifyIP(lpszIP, lpszNetmask, lpszGateway) && ModifyDns(lpszDns1, lpszDns2);
}

void  WMILib::CreateOneElementBstrArray(VARIANT*  v,  LPCWSTR  s)    
{    
	SAFEARRAYBOUND  bound[1];    
	SAFEARRAY*  array;    
	bound[0].lLbound  =  0;    
	bound[0].cElements  =  1;    
	array  =  SafeArrayCreate(VT_BSTR,  1,  bound);    
	long  index  =  0;    
	BSTR  bstr  =  SysAllocString(s);    
	SafeArrayPutElement(array,  &index,  bstr);    
	SysFreeString(bstr);    
	VariantInit(v);    
	v->vt  =  VT_BSTR    |  VT_ARRAY;    
	v->parray  =  array;    
}

void WMILib::PrintWMIError(HRESULT hr) {
	IWbemStatusCodeText * pStatus = NULL;
	HRESULT hres = CoCreateInstance(CLSID_WbemStatusCodeText, 0,
		CLSCTX_INPROC_SERVER, IID_IWbemStatusCodeText, (LPVOID *) &pStatus);
	if (S_OK == hres) {
		BSTR bstrError;
		hres = pStatus->GetErrorCodeText(hr, 0, 0, &bstrError);
		if (S_OK != hres)
			bstrError = SysAllocString(L"Get last error failed");
		BSTR bstrTError = bstrError;
		wchar_t* lpszError = bstrTError;
		printf("WMI Error:%s\n", lpszError);
		pStatus->Release();
		SysFreeString(bstrError);
	}
}
