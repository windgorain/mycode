
function MYUTL_Assert(bExp, sMsg)
{
    if ( !bExp)
    {
        alert ("ASSERT: " + sMsg);
    }

    return true;
}

/**
 * ������C�����е�sprintf�����Ը�ʽ���ַ���
*/
function MYUTL_Sprintf(sFormat, sValuelist)
{
    var sTemp;
    if (arguments.length == 1)
        return sFormat;
    
    var arrTmp = new Array();
    /**
     * ����Ϊһ���ַ����������ʽ
     */
    if (typeof(sValuelist) == "object")
    {
        arrTmp = sValuelist;
    }
    /**
     * ����Ϊ����ַ��������ֵ���ʽ
     */
    else
    {
        for (var j=1; j<arguments.length; j++)
        {
            arrTmp[j-1] = arguments[j];
        }
    }
        
    var sRet = "";
    for ( var i=0; ;i++ )
    {
        var n = sFormat.indexOf("%");
        if ( n==-1 )
        {
            break;
        }
        sRet += sFormat.substring(0,n);
        var ch = sFormat.charAt(n+1);
        switch ( ch )
        {
        case '%':
            sRet += "%";
            break;
        case 's':
            sRet += i<arrTmp.length ? arrTmp[i] : "%s";
            break;
        case 'x':
        case 'X':
            if(i<arrTmp.length )
            {
                sTemp = parseInt(arrTmp[i]).toString(16);
                if('X' == ch)
                {
                    sTemp = sTemp.toUpperCase();
                }
                sRet += sTemp;
            }
            else
            {
                sRet += "%"+ch;
            }
            break;
        case 'd':
            sRet += i<arrTmp.length ? parseInt(arrTmp[i]) : "%d";
            break;
        default:
            sRet += "%"+ch;
            break;
        }
        sFormat = sFormat.substring(n+2);
    }
    sRet += sFormat;
    return sRet;
}

/* ���sDst��Ϊ��, ����Ϸָ�����sSrc. ����ֱ�Ӹ�ֵΪsSrc */
function MYUTL_AddString(sDst, sSrc, sSplit)
{
	if (sDst.length == 0)
	{
		return sSrc;
	}

	return sDst + sSplit + sSrc;
}

/* �ж�sEle�Ƿ���sList��, �ָ���ΪsSplit */
function MYUTL_IsInclude(sList, sEle, sSplit)
{
	var aEles = sList.split(sSplit);
	
	for (var index in aEles)
	{
		if (aEles[index] == sEle)
		{
			return true;
		}
	}
	
	return false;
}

/* �����ַ������%xx */
function MYUTL_Encode(sString)
{
	sString = encodeURI(sString);
	sString = sString.replaceAll("&", "%26");
	
	return sString;
}