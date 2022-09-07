/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-8
* Description: һЩIp��صĳ��ú���
* History:     
******************************************************************************/

/* 
	��IPת��Ϊ4��int������.
	ʧ���򷵻�null
*/
function IP_IP2Int4(sIP)
{
	/**
     * ��Ч��У��
     */
    var IPPattern = /^\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}$/
    if(!IPPattern.test(sIP))
	{
		return null;
    }

	var IPArray = sIP.split(".");

    if (IPArray.length != 4)
	{
		return null;
    }

    var ip1 = parseInt(IPArray[0],10);
    var ip2 = parseInt(IPArray[1],10);
    var ip3 = parseInt(IPArray[2],10);
    var ip4 = parseInt(IPArray[3],10);

    if ( ip1<0 || ip1>255
        || ip2<0 || ip2>255
        || ip3<0 || ip3>255
        || ip4<0 || ip4>255 )
    {
        return null;
    }

	return [ip1,ip2,ip3,ip4];
}


/**
 * �������IP��ַ����У��,��ʽΪxx.xx.xx.xx
 */
function IP_ValidateIP4(sIP)
{
    var IPArray = IP_IP2Int4(sIP);
	if (null == IPArray)
	{
		return false;
	}

    if ((IPArray[0]+IPArray[1]+IPArray[2]+IPArray[3])==0 )
    {
        /**
         * the value is 0.0.0.0
         */
        return false;
    }

    return true;
}

function IP_IsPrefixLen(sMask)
{
	for (var i=0; i<33; i++)
	{
		if (i == sMask)
		{
			return true;
		}
	}

	return false;
}

/*
	�������뷵�����볤��
	����255.255.0.0, �򷵻�16.
	������33
*/
function IP_Mask2PrefixLen(sMask)
{
    /*ȥ��ǰ���0*/
    sMask = sMask.replace(/^[0]*([0-9]+)\.[0]*([0-9]+)\.[0]*([0-9]+)\.[0]*([0-9]+)$/,"$1.$2.$3.$4");
    
    var aIPMask = ["0.0.0.0","128.0.0.0", "192.0.0.0", "224.0.0.0", "240.0.0.0", "248.0.0.0", "252.0.0.0",
                   "254.0.0.0", "255.0.0.0", "255.128.0.0", "255.192.0.0", "255.224.0.0", "255.240.0.0",
                   "255.248.0.0", "255.252.0.0", "255.254.0.0", "255.255.0.0", "255.255.128.0","255.255.192.0",
                   "255.255.224.0", "255.255.240.0", "255.255.248.0", "255.255.252.0", "255.255.254.0", "255.255.255.0",
                   "255.255.255.128", "255.255.255.192", "255.255.255.224", "255.255.255.240", "255.255.255.248", 
                   "255.255.255.252", "255.255.255.254", "255.255.255.255"];

    for (var i=0; i <= 32; i ++)
    {   
        if (aIPMask[i] == sMask)
        {
            return i;
        }
    }

    return 33;
}

function IP_PrefixLen2Mask(uiPrefixLen)
{
    var aIPMask = ["0.0.0.0","128.0.0.0", "192.0.0.0", "224.0.0.0", "240.0.0.0", "248.0.0.0", "252.0.0.0",
                   "254.0.0.0", "255.0.0.0", "255.128.0.0", "255.192.0.0", "255.224.0.0", "255.240.0.0",
                   "255.248.0.0", "255.252.0.0", "255.254.0.0", "255.255.0.0", "255.255.128.0","255.255.192.0",
                   "255.255.224.0", "255.255.240.0", "255.255.248.0", "255.255.252.0", "255.255.254.0", "255.255.255.0",
                   "255.255.255.128", "255.255.255.192", "255.255.255.224", "255.255.255.240", "255.255.255.248", 
                   "255.255.255.252", "255.255.255.254", "255.255.255.255"];

	if (uiPrefixLen > 32)
	{
		return null;
	}

	return aIPMask[uiPrefixLen];
}

/* ���IP�Ƿ���һ���������� */
function IP_CheckOneSubNet(sIp1, sIp2, sMask)
{
	var IPArray1 = IP_IP2Int4(sIp1);
	if (null == IPArray1)
	{
		return false;
	}

	var IPArray2 = IP_IP2Int4(sIp2);
	if (null == IPArray2)
	{
		return false;
	}

	var MaskArray = IP_IP2Int4(sMask);
	if (null == MaskArray)
	{
		return false;
	}

	if (((IPArray1[0] & MaskArray[0]) != (IPArray2[0] & MaskArray[0]))
		|| ((IPArray1[1] & MaskArray[1]) != (IPArray2[1] & MaskArray[1]))
		|| ((IPArray1[2] & MaskArray[2]) != (IPArray2[2] & MaskArray[2]))
		|| ((IPArray1[3] & MaskArray[3]) != (IPArray2[3] & MaskArray[3])))
	{
		return false;
	}

	return true;
}

