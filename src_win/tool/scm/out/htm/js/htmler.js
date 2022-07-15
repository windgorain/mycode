function _string_2_json(sString) /* 将字符串转换为Json */
{
	var obj = new Object();

	obj.string = sString;

	if (sString == "")
	{
		sString = "{}";
	}
	
	obj.json = $.parseJSON(sString);
	
	return obj;
}

function Htmler_IsSupport()/* 是否支持htmler */
{
	try
	{
		if (HtmlerCmd('CheckSupport'))
		{
			return true;
		}
		
		return false;
	}
	catch(e)
	{
		return false;
	}
}

function HtmlerEncode(sActions)
{
	sActions = sActions.replace(/%/g, "%25");
	var s = sActions.replace(/,/g, "%2c");
	s = s.replace(/;/g, "%3b");
	return s;
}

function HtmlerCmd(sAction, sParam)
{
	var sCmd;
	
	if (! arguments[1])
	{
		sCmd = "Action=" + sAction;
	}
	else
	{
		sCmd = "Action=" + sAction + "," + sParam;
	}
	
	var sRet = window.external.HtmlerCmd(sCmd);
	return _string_2_json(sRet);
}

function BsLoad()
{
	HtmlerCmd("DllCall", "File=bs.dll,Func=LoadBs_Init");
}

function BsDo(sDo, sParam)
{
	var sCmd;

	if (! arguments[1])
	{
		sCmd = 'File=bs.dll,Func=KFBS_Do,Cmd=_do=' + sDo;
	}
	else
	{
		sCmd = 'File=bs.dll,Func=KFBS_Do,Cmd=_do=' + sDo + '&' + sParam
	}

	return HtmlerCmd('DllCall', sCmd);
}