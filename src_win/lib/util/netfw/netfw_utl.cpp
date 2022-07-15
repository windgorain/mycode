/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-3-17
* Description: windows·À»ðÇ½½Ó¿Ú
* History:     
******************************************************************************/
#include "bs.h"
#include <netfw.h>

#pragma comment( lib, "ole32.lib" )
#pragma comment( lib, "oleaut32.lib" )


BS_STATUS WindowsFirewallInitialize(OUT HANDLE *phFwProfile)
{
    HRESULT hr = S_OK;
    INetFwMgr* fwMgr = NULL;
    INetFwPolicy* fwPolicy = NULL;
    HRESULT comInit = E_FAIL;


    *phFwProfile = NULL;

    // Initialize COM.
    comInit = CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    // Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
    // initialized with a different mode. Since we don't care what the mode is,
    // we'll just use the existing mode.
    if (comInit != RPC_E_CHANGED_MODE)
    {
        if (FAILED(comInit))
        {
            return BS_ERR;
        }
    }

    // Create an instance of the firewall settings manager.
    hr = CoCreateInstance(__uuidof(NetFwMgr), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void**)&fwMgr);
    if (FAILED(hr))
    {
        return BS_ERR;
    }

    // Retrieve the local firewall policy.
    hr = fwMgr->get_LocalPolicy(&fwPolicy);
    if (FAILED(hr))
    {
        fwMgr->Release();
        return BS_ERR;
    }

    // Retrieve the firewall profile currently in effect.
    hr = fwPolicy->get_CurrentProfile((INetFwProfile **)phFwProfile);
    if (FAILED(hr))
    {
        fwPolicy->Release();
        fwMgr->Release();
        return BS_ERR;
    }

    fwPolicy->Release();
    fwMgr->Release();

    return BS_OK;
}


VOID WindowsFirewallCleanup(IN HANDLE hFwProfile)
{
	INetFwProfile *pFwProfile = (INetFwProfile *)hFwProfile;

    // Release the firewall profile.
    if (pFwProfile != NULL)
    {
        pFwProfile->Release();
    }
}

BS_STATUS WindowsFirewallIsOn(IN HANDLE hFwProfile, OUT BOOL_T *bfwOn)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL fwEnabled;
	INetFwProfile *pFwProfile = (INetFwProfile *)hFwProfile;

    *bfwOn = FALSE;

    // Get the current state of the firewall.
    hr = pFwProfile->get_FirewallEnabled(&fwEnabled);
    if (FAILED(hr))
    {
        return BS_ERR;
    }

    // Check to see if the firewall is on.
    if (fwEnabled != VARIANT_FALSE)
    {
        *bfwOn = TRUE;
    }

    return BS_OK;
}

BS_STATUS WindowsFirewallTurnOn(IN HANDLE hFwProfile)
{
    HRESULT hr = S_OK;
    BOOL_T bFwOn;
	INetFwProfile *pFwProfile = (INetFwProfile *)hFwProfile;

    // Check to see if the firewall is off.
    hr = WindowsFirewallIsOn(pFwProfile, &bFwOn);
    if (FAILED(hr))
    {
        return BS_ERR;
    }

    if (bFwOn == TRUE)
    {
        return BS_OK;
    }

    // Turn the firewall on.
    hr = pFwProfile->put_FirewallEnabled(VARIANT_TRUE);
    if (FAILED(hr))
    {
        return BS_ERR;
    }

    return BS_OK;
}


BS_STATUS WindowsFirewallTurnOff(IN HANDLE hFwProfile)
{
    HRESULT hr = S_OK;
    BOOL_T bFwOn;
	INetFwProfile *pFwProfile = (INetFwProfile *)hFwProfile;

    // Check to see if the firewall is on.
    hr = WindowsFirewallIsOn(pFwProfile, &bFwOn);
    if (FAILED(hr))
    {
        return BS_ERR;
    }

    if (bFwOn == FALSE)
    {
        return BS_OK;
    }

    hr = pFwProfile->put_FirewallEnabled(VARIANT_FALSE);
    if (FAILED(hr))
    {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS WindowsFirewallAppIsEnabled
(
    IN HANDLE hFwProfile,
    IN const wchar_t* fwProcessImageFileName,
    OUT BOOL_T *bFwAppEnabled
)
{
    HRESULT hr = S_OK;
    BSTR fwBstrProcessImageFileName = NULL;
    VARIANT_BOOL fwEnabled;
    INetFwAuthorizedApplication* fwApp = NULL;
    INetFwAuthorizedApplications* fwApps = NULL;
	INetFwProfile *pFwProfile = (INetFwProfile *)hFwProfile;

    *bFwAppEnabled = FALSE;

    // Retrieve the authorized application collection.
    hr = pFwProfile->get_AuthorizedApplications(&fwApps);
    if (FAILED(hr))
    {
        return BS_ERR;
    }

    // Allocate a BSTR for the process image file name.
    fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);
    if (fwBstrProcessImageFileName == NULL)
    {
        fwApps->Release();
        return BS_ERR;
    }

    // Attempt to retrieve the authorized application.
    hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp);
    if (! SUCCEEDED(hr))
    {
        SysFreeString(fwBstrProcessImageFileName);
        fwApps->Release();
        return BS_OK;
    }

    // Find out if the authorized application is enabled.
    hr = fwApp->get_Enabled(&fwEnabled);
    if (FAILED(hr))
    {
        SysFreeString(fwBstrProcessImageFileName);
        fwApp->Release();
        fwApps->Release();
        return BS_ERR;
    }

    if (fwEnabled != VARIANT_FALSE)
    {
        // The authorized application is enabled.
        *bFwAppEnabled = TRUE;
    }

    SysFreeString(fwBstrProcessImageFileName);
    fwApp->Release();
    fwApps->Release();

    return BS_OK;
}


BS_STATUS WindowsFirewallAddApp
(
    IN HANDLE hFwProfile,
    IN const wchar_t* fwProcessImageFileName,
    IN const wchar_t* fwName
)
{
    HRESULT hr = S_OK;
    BOOL_T fwAppEnabled;
    BSTR fwBstrName = NULL;
    BSTR fwBstrProcessImageFileName = NULL;
    INetFwAuthorizedApplication* fwApp = NULL;
    INetFwAuthorizedApplications* fwApps = NULL;
	INetFwProfile *pFwProfile = (INetFwProfile *)hFwProfile;

    // First check to see if the application is already authorized.
    hr = WindowsFirewallAppIsEnabled(pFwProfile, fwProcessImageFileName, &fwAppEnabled);
    if (FAILED(hr))
    {
        return BS_ERR;
    }

    if (fwAppEnabled == TRUE)
    {
        return BS_OK;
    }

    // Retrieve the authorized application collection.
    hr = pFwProfile->get_AuthorizedApplications(&fwApps);
    if (FAILED(hr))
    {
        return BS_ERR;
    }

    // Create an instance of an authorized application.
    hr = CoCreateInstance(__uuidof(NetFwAuthorizedApplication), NULL, CLSCTX_INPROC_SERVER,
                        __uuidof(INetFwAuthorizedApplication), (void**)&fwApp);
    if (FAILED(hr))
    {
        fwApps->Release();
        return BS_ERR;
    }

    // Allocate a BSTR for the process image file name.
    fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);
    if (fwBstrProcessImageFileName == NULL)
    {
        fwApp->Release();
        fwApps->Release();
        return BS_ERR;
    }

    // Set the process image file name.
    hr = fwApp->put_ProcessImageFileName(fwBstrProcessImageFileName);
    if (FAILED(hr))
    {
        SysFreeString(fwBstrProcessImageFileName);
        fwApp->Release();
        fwApps->Release();
        return BS_ERR;
    }

    // Allocate a BSTR for the application friendly name.
    fwBstrName = SysAllocString(fwName);
    if (SysStringLen(fwBstrName) == 0)
    {
        SysFreeString(fwBstrProcessImageFileName);
        fwApp->Release();
        fwApps->Release();
        return BS_ERR;
    }

    // Set the application friendly name.
    hr = fwApp->put_Name(fwBstrName);
    if (FAILED(hr))
    {
        SysFreeString(fwBstrName);
        SysFreeString(fwBstrProcessImageFileName);
        fwApp->Release();
        fwApps->Release();
        return BS_ERR;
    }

    // Add the application to the collection.
    hr = fwApps->Add(fwApp);
    if (FAILED(hr))
    {
        SysFreeString(fwBstrName);
        SysFreeString(fwBstrProcessImageFileName);
        fwApp->Release();
        fwApps->Release();
        return BS_ERR;
    }

    SysFreeString(fwBstrName);
    SysFreeString(fwBstrProcessImageFileName);
    fwApp->Release();
    fwApps->Release();

    return BS_OK;
}

BS_STATUS WindowsFirewallPortIsEnabled
(
    IN HANDLE hFwProfile,
    IN LONG portNumber,
    IN NET_FW_IP_PROTOCOL ipProtocol,
    OUT BOOL_T * bFwPortEnabled
)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL fwEnabled;
    INetFwOpenPort* fwOpenPort = NULL;
    INetFwOpenPorts* fwOpenPorts = NULL;
	INetFwProfile *pFwProfile = (INetFwProfile *)hFwProfile;

    *bFwPortEnabled = FALSE;

    // Retrieve the globally open ports collection.
    hr = pFwProfile->get_GloballyOpenPorts(&fwOpenPorts);
    if (FAILED(hr))
    {
        return BS_ERR;
    }

    // Attempt to retrieve the globally open port.
    hr = fwOpenPorts->Item(portNumber, ipProtocol, &fwOpenPort);
    if (! SUCCEEDED(hr))
    {
        fwOpenPorts->Release();
        return BS_OK;
    }

    // Find out if the globally open port is enabled.
    hr = fwOpenPort->get_Enabled(&fwEnabled);
    if (FAILED(hr))
    {
        fwOpenPort->Release();
        fwOpenPorts->Release();
        return BS_ERR;
    }

    if (fwEnabled != VARIANT_FALSE)
    {
        // The globally open port is enabled.
        *bFwPortEnabled = TRUE;
    }

    fwOpenPort->Release();
    fwOpenPorts->Release();
    return BS_OK;
}

BS_STATUS WindowsFirewallPortAdd
(
    IN HANDLE hFwProfile,
    IN LONG portNumber,
    IN NET_FW_IP_PROTOCOL ipProtocol,
    IN const wchar_t* name
)
{
    HRESULT hr = S_OK;
    BOOL_T fwPortEnabled;
    BSTR fwBstrName = NULL;
    INetFwOpenPort* fwOpenPort = NULL;
    INetFwOpenPorts* fwOpenPorts = NULL;
	INetFwProfile *pFwProfile = (INetFwProfile *)hFwProfile;

    // First check to see if the port is already added.
    hr = WindowsFirewallPortIsEnabled(pFwProfile, portNumber, ipProtocol, &fwPortEnabled);
    if (FAILED(hr))
    {
        return BS_ERR;
    }

    if (fwPortEnabled == TRUE)
    {
        return BS_OK;
    }

    // Retrieve the collection of globally open ports.
    hr = pFwProfile->get_GloballyOpenPorts(&fwOpenPorts);
    if (FAILED(hr))
    {
        return BS_ERR;
    }

    // Create an instance of an open port.
    hr = CoCreateInstance(__uuidof(NetFwOpenPort), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwOpenPort), (void**)&fwOpenPort);
    if (FAILED(hr))
    {
        fwOpenPorts->Release();
        return BS_ERR;
    }

    // Set the port number.
    hr = fwOpenPort->put_Port(portNumber);
    if (FAILED(hr))
    {
        fwOpenPort->Release();
        fwOpenPorts->Release();
        return BS_ERR;
    }

    // Set the IP protocol.
    hr = fwOpenPort->put_Protocol(ipProtocol);
    if (FAILED(hr))
    {
        fwOpenPort->Release();
        fwOpenPorts->Release();
        return BS_ERR;
    }

    // Allocate a BSTR for the friendly name of the port.
    fwBstrName = SysAllocString(name);
    if (SysStringLen(fwBstrName) == 0)
    {
        fwOpenPort->Release();
        fwOpenPorts->Release();
        return BS_ERR;
    }

    // Set the friendly name of the port.
    hr = fwOpenPort->put_Name(fwBstrName);
    if (FAILED(hr))
    {
        SysFreeString(fwBstrName);
        fwOpenPort->Release();
        fwOpenPorts->Release();
        return BS_ERR;
    }

    // Opens the port and adds it to the collection.
    hr = fwOpenPorts->Add(fwOpenPort);
    if (FAILED(hr))
    {
        SysFreeString(fwBstrName);
        fwOpenPort->Release();
        fwOpenPorts->Release();
        return BS_ERR;
    }

    SysFreeString(fwBstrName);
    fwOpenPort->Release();
    fwOpenPorts->Release();

    return BS_OK;
}


