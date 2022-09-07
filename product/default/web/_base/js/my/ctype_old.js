
/**
 * Desc:
	完成FORM中的一些自动化操作, 只对<input type=text>有效

 * 支持的属性
	1. ctype - 每种取值会有不同的属性对应，分别如下：
		mask			IPV4格式的字符串, 允许输入0.0.0.0
		mac				MAC地址字符串, 格式为HHHH-HHHH-HHHH. 允许输入0-0-0
		lumac			MAC地址字符串, 格式为HH-HH-HH-HH-HH-HH
		ip4				IPV4格式的字符串, 不允许输入0.0.0.0
		ip4pz			IPV4格式的字符串, 允许输入0.0.0.0
		int				十进制整数, 前导空格和0将被忽略, 不支持0x的十六进制格式.
		newint			十进制整数, 前导空格和0将被忽略, 不支持0x的十六进制格式.增加属性Multiple="2",即检查是否为2的倍数
		hStr			十六进制字符串, 只允许字符 0-9 a-f A-F
		word			不能包含中文和?<>\"%'&#和空格
		str				不能包含中文和?<>\"%'&#, 该类型可以不写
		text			不能包含下列任何字符之一 ? < > \ " % ' & # 
		password
		str_name   		过滤特殊字符：？、中文
		str_urlfil		过滤特殊字符：？、中文、空格
		ip6				ipv6 地址
		wildcard 		通配符
		custom			自定义
		email			E-Mail
		limit_str		只允许输入数字 字母 _ -


	2. ErrMsg - 错误字符串， 用于检查不通过时显示给用户， 如果没有则显示默认提示。

	3. min and max.
		min - 最小值，如果没有则不限制最小值
		max - 最大值，如果没有则不限制最大值
		根据wType的值不同有不同的意义
		1). int, 此时表示数值的最大值和最小值
		2). hStr, word, str, 此时表示字符串的最大长度和最小长度, 如果某一输入框
			为必须输入, 可以通过设置最小长度为大于0的数实现

 * 导出函数表
	CType_CheckForm
	
 */

var CTypeRc = {};

CTypeRc.RS_ERROR = "错误";
CTypeRc.RS_INPUT_NOT_ALLOW_URLFIL = "请输入长度在%d－%d范围内的字符串，并且不能包含‘？’、空格和中文字符。";
CTypeRc.RS_INPUT_NOT_ALLOW_URL = "请输入长度在%d－%d范围内的字符串，并且不能包含中文字符。";
CTypeRc.RS_INPUT_NOT_ALLOW_DOMAIN = "请输入长度在%d－%d范围内的字符串，并且不能包含‘？’、‘\\’、‘/’、‘@’、‘：’、‘*’、‘“’、‘|’、‘<’、‘>’和中文字符。";//用于msr的wlan模块的domain
CTypeRc.RS_ERR_NOT_MULTIPLE = "只允许输入由0-9组成的十进制数，并且是%d的倍数。";
CTypeRc.RS_INPUT_NOT_ALLOW_SPACE = "请输入长度在%d－%d范围内的字符串，并且不能包含‘？’、‘～’、空格和中文字符。";
CTypeRc.RS_INPUT_NOT_ALLOW_NAME = "请输入长度在%d－%d范围内的字符串，并且不能包含‘？’和中文字符。";
CTypeRc.RS_INPUT_NOT_ALLOW_PASSWORD = "请输入长度在%d－%d范围内或88个字符串，并且不能包含‘？’和中文字符。";
CTypeRc.RS_INPUT_NOT_ALLOW_STRING = "请输入长度在%d－%d范围内的字符串，并且不能包含‘/’、‘\\’、‘：’、‘|’、‘@’、‘*’、‘？’、‘“’、‘<’、‘>’和中文字符。";

CTypeRc.RS_INPUT_NOT_PVC_TYPE = "请输入PVC的VPI/VCI值对，VPI和VCI值均为%d－%d范围内的整数，而且不能同时为0。";
CTypeRc.RS_INPUT_NOT_TIMESLOT_TYPE = "请输入时隙分组值，例如1，3-6，时隙分组值为%d－%d范围内的整数。";
CTypeRc.RS_INPUT_NOT_ALLOW = "有非法字符‘%s’输入。";
CTypeRc.RS_UNSUPPORTED_CHAR_EN = "不能包含中文和下列任何字符之一\r\n\r\n%s";
CTypeRc.RS_UNSUPPORTED_CHAR = "不能包含下列任何字符之一\r\n\r\n%s";
CTypeRc.RS_NO_SPACE = "不允许输入空格。";
CTypeRc.RS_ERR_NOT_IN_RANGE = "输入值不在指定的范围内。";
CTypeRc.RS_ERR_NOT_IN_INT_RANGE = "请输入%d－%d范围内的整数。";
CTypeRc.RS_ERR_NOT_IN_STR_RANGE = "请输入长度在%d－%d范围内的字符。";
CTypeRc.RS_ERR_NOT_EQUAL_STR_LENGTH = "请输入长度为%d的字符串。";
CTypeRc.RS_ERR_IP4 = "IP地址不合法。";
CTypeRc.RS_ERR_IP4PZ = "IP地址不合法。";
CTypeRc.RS_ERR_MASK = "掩码必须符合点分十进制的格式。";
CTypeRc.RS_ERR_MASK_ERROR = "请输入点分十进制格式的掩码（%s－%s）或输入掩码长度（%d－%d）。";
CTypeRc.RS_ERR_SERIES_MASK_ERROR = "请配置连续掩码。";
CTypeRc.RS_ERR_MAC = "MAC地址的输入格式应为H-H-H。";
CTypeRc.RS_ERR_MAC_STRICT = "无效的MAC地址。";
CTypeRc.RS_ERR_UNIMAC = "请输入格式为H-H-H的单播MAC地址。";
CTypeRc.RS_ERR_LUMAC = "请输入格式为HH-HH-HH-HH-HH-HH的MAC地址。";
CTypeRc.RS_ERR_NOT_NUMBER = "只允许输入整数。";
CTypeRc.RS_ERR_ONLY_HEX_CHAR = "只允许输入十六进制数。";
CTypeRc.RS_ERR_ONLY_DEC_CHAR = "只允许输入由0-9组成的字符串。";
CTypeRc.RS_ERR_IP6 = "IPv6地址不合法。";
CTypeRc.RS_ERR_NOT_EMPTY = "不允许输入空。";
CTypeRc.RS_ERR_SELVLAN_FIRST = "请先选择要操作的VLAN范围。";
CTypeRc.RS_ERR_QUOTATION = "输入错误。";
CTypeRc.RS_LOOPBACK_MASK = "LoopBack端口仅支持32位掩码模式，要换成32位掩码吗？";
CTypeRc.RS_ERR_WLANSTR = "只允许输入由0-9和a-z（A-Z）组成的字符串。";
CTypeRc.RS_ERR_COLON = "：";
CTypeRc.RS_ERR_PASSWORD = "密码错误：密码不能以空格开头或结尾。";
CTypeRc.RS_MENU_CWMP = "CWMP";
CTypeRc.RS_MENU_CWMP_DESC = "CWMP";
CTypeRc.RS_TAB_CWMPSUMMARY = "显示";
CTypeRc.RS_TAB_CWMPSUMMARY_DESC = "显示当前CWMP配置";
CTypeRc.RS_TAB_CWMPSETUP = "设置";
CTypeRc.RS_TAB_CWMPSETUP_DESC = "设置CWMP参数";
CTypeRc.RS_ERR_EMAIL = "邮件地址格式不合法。";
CTypeRc.RS_ERR_NOT_NEXTHOPEMPTY = "下一跳地址不能为空"; /* Added by heshan for WLD29602 WLD30732 WLD30562 */
CTypeRc.RS_ERR_NOT_IFNEXTHOPBOTHEMPTY = "出接口和下一跳地址不能同时为空";
CTypeRc.RS_ERR_LIMIT_STR = "只允许输入由数字、字母、_、-组成的字符串";




var CType;
if (CType && (typeof CType != "object" || CType.NAME))
	throw new Error("Namespace 'CType' already exists");

/**
 * 创建名字空间 : CType
 */
CType = {};

/**
 * 添加此名字空间的后续的一些信息
 */
CType.NAME = "CType";  // 名字
CType.VERSION = "0.1";	// 版本

/**
 * 此匿名函数定义后直接调用，导出公共函数到Form名字空间
 * 导出函数表
	1. checkInputForm(oForm)
 */
(function(){
// Public:

	/**
	 * 导出公共对外函数到Form名字空间
	 */
	CType.checkInputForm = checkInputForm; 
	CType.checkInputElement = checkInputElement;

// Private:

	/**
	 * 不支持的特殊字符列表.
	 */
	var UNSUPPORT_CHAR = "?<>\\\"%'&#";

	/**
	 * 采用闭包的方法，使得私有数据和函数真正私有，不受外界影响，也不影响外界(尤其是全局命名空间)
	 * 采用约定的下划线开始的变量和函数命名约定方式，不能真正保护内部数据，而且会扰乱全局命名空间
	 */

	var bAlertBox = false; /* 是否弹出提示框 */

	var pfTbl = [];
	pfTbl["mask"] = checkMask;
	pfTbl["mac"] = checkMac;
	pfTbl["lumac"] = checkLUMac;
	pfTbl["ip4"] = checkIP4;
	pfTbl["ip4pz"] = checkIP4PZ;
	pfTbl["int"] = checkNumber;
	pfTbl["str"] = checkString;
	pfTbl["text"] = checkText;
	pfTbl["hStr"] = checkHexStr;
	pfTbl["ip6"] = checkIP6;
	pfTbl["word"] = checkWord;
	pfTbl["password"] = checkPassword;
	pfTbl["custom"] = function(oEle){return true;};
	pfTbl["str_name"] = checkString_name;
	pfTbl["str_urlfil"] = checkString_urlfil;
	pfTbl["newint"] = checkNewNumber;
	pfTbl["wildcard"] = checkWildcard;
	pfTbl["email"] = CheckEmail;
	pfTbl["limit_str"] = CheckLimitStr;
	

function checkInputForm(oForm)
{
	  var oInputs = oForm.getElementsByTagName ("input");

	for ( var i=0; i<oInputs.length; i++ )
	{
		if(!checkInputElement(oInputs[i]))
			return false;
	}
	return true;
}

function checkInputElement(oEle)
{
	var sEleType;
	var cType = oEle.getAttribute("ctype");
	var sCtypeInfo = oEle.getAttribute("ctype_info");
	var oCtypeInfo = null;

	if (sCtypeInfo)
	{
		oCtypeInfo = document.getElementById(sCtypeInfo);
		if (oCtypeInfo)
		{
			oCtypeInfo.innerHTML = "";
		}
	}


	/**
	 * 只检查input输入框
	 * 只对enable的输入框作处理
	 */
	if (((oEle.tagName.toLowerCase() != "input") && (oEle.tagName.toLowerCase() != "textarea"))
		|| oEle.disabled
		|| oEle.type == undefined
		|| ((oEle.type.toLowerCase()!="text")&&cType==null)
		|| cType == "skip")
	{
		return true;
	}

	/**
	 * 开始检查。对特殊的情况不能删除前后空格：
	 * 1。密码可以输入各种字符，不需要删除空格
	 * 2。文件在某些浏览器上删除空格时会出错，因此需要跳过
	 * 3。wType="text"时，为最宽条件支持，也不需要删除空格
	 */
	
	sEleType = oEle.type.toLowerCase();
	if(sEleType!="password" && sEleType!="file" && cType!="text")/* "file"为浏览器兼容增加 */
	{
		trim(oEle);
	}

	if (oEle.value == "")
	{
		if (oEle.getAttribute("empty") != "true" )
		{
			ErrorBox(oEle, CTypeRc.RS_ERR_NOT_EMPTY);
			return false;
		}
		else return true;
	}

	var pfCheck = (null != pfTbl[cType])?pfTbl[cType]:pfTbl["str"];
	if(!pfCheck(oEle)) return false;

	var checkCB = oEle.getAttribute("checkCB");
	if (null != checkCB && checkCB.length > 0)
	{
		checkCB = checkCB.replace("this",'oEle');
		var errStr = eval("try{oEle.ownerDocument.parentWindow." + checkCB + "}catch(e){GetTop().content.mainframe." + checkCB + "}");
		if(null != errStr)
		{
			if(errStr.length > 0)
			{
				ErrorBox(oEle, errStr);
			}
			return false;
		}
	}

	return true;
}

/**
 * 对输入的IP地址进行校验,格式为xx.xx.xx.xx
 */
function validateIP4(IPstr)
{
	/**
	 * 有效性校验
	 */
	var IPPattern = /^\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}$/
	if(!IPPattern.test(IPstr))return false;

	/**
	 * 检查域值
	 */
	var IPArray = IPstr.split(".");
	if(IPArray.length != 4)return false;

	var ip1 = parseInt(IPArray[0],10);
	var ip2 = parseInt(IPArray[1],10);
	var ip3 = parseInt(IPArray[2],10);
	var ip4 = parseInt(IPArray[3],10);
	/**
	 * 每个域值范围0-255
	 */
	if ( ip1<0 || ip1>255
		|| ip2<0 || ip2>255
		|| ip3<0 || ip3>255
		|| ip4<0 || ip4>255 )
	{
		return false;
	}

	if ( (ip1+ip2+ip3+ip4)==0 )
	{
		/**
		 * the value is 0.0.0.0
		 */
		return false;
	}

	return true;
}

/**
 * 对输入的IP地址进行校验,格式为xx.xx.xx.xx
 */
function validateMask(oEle)
{
	var MaskStr = oEle.value;

	/**
	 * 有效性校验
	 */
	var IPPattern = /^\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}$/

	if ( MaskStr == "0" )
	{
		oEle.value = "0.0.0.0";
		return true;
	}
	if(!IPPattern.test(MaskStr))return false;

	/**
	 * 检查域值
	 */
	var IPArray = MaskStr.split(".");
	var ip1 = parseInt(IPArray[0]);
	var ip2 = parseInt(IPArray[1]);
	var ip3 = parseInt(IPArray[2]);
	var ip4 = parseInt(IPArray[3]);
	/**
	 * 每个域值范围0-255
	 */
	if ( ip1<0 || ip1>255
		|| ip2<0 || ip2>255
		|| ip3<0 || ip3>255
		|| ip4<0 || ip4>255 )
	{
		return false;
	}

	return true;
}

function ErrorBox(oEle, sDefaultMsg)
{
	var sMsg = oEle.getAttribute("ErrMsg");
	var sCtypeInfo = oEle.getAttribute("ctype_info");
	var oCtypeInfo = null;

	if (sCtypeInfo)
	{
		oCtypeInfo = document.getElementById(sCtypeInfo);
	}
	
	if ( !sMsg )
	{
		sMsg = sDefaultMsg;
	}
	else
	{
		sMsg += CTypeRc.RS_ERROR + CTypeRc.RS_SEPERATOR + sDefaultMsg;
	}

	sMsg = sMsg.replace( (new RegExp(CTypeRc.RS_ERROR + CTypeRc.RS_ERROR,"g")), CTypeRc.RS_ERROR);

	if (oCtypeInfo)
	{
		oCtypeInfo.innerHTML = sMsg;
	}

	if (bAlertBox == true)
	{
		/**
		 * type为hidden情况下，oEle.focus()会异常，导致无法打提示信息。因此做try处理。
		 */
		try
		{
			oEle.select();
			oEle.focus();
		}
		catch(e)
		{
		}

		alert(sMsg);
	}

	return true;
}

function checkRange(oEle, nValue)
{
	/**
	 * 因为是常量，所以忽略非数字的检查
	 */
	var nMin = parseInt(oEle.getAttribute("min"));
	var nMax = parseInt(oEle.getAttribute("max"));
	/**
	 * 检查最小值 最大值
	 */
	if ( (!isNaN(nMin) && nValue<nMin)
		||(!isNaN(nMax) && nValue>nMax)
		)
	{
		ErrorBox(oEle, CTypeRc.RS_ERR_NOT_IN_RANGE);//"NOT in range.");
		return false;
	}

	return true;
}

/**
 * 自动去掉Form中所有输入框的前后空格.
 */
function trim(oEle)
{
	oEle.value = oEle.value.trim();
	return true;
}

/**
 * 检查Form中所有的IP是否输入了正确的格式
 */
function checkIP4(oEle)
{
	var IPstr = oEle.value;
	if ( !validateIP4(IPstr) )
	{
		ErrorBox(oEle, CTypeRc.RS_ERR_IP4);
		return false;
	}

	/**
	 *	for 0xx.0xx.0xx.0xx
	 */
	oEle.value = IPstr.replace(/^[0]*([0-9]+)\.[0]*([0-9]+)\.[0]*([0-9]+)\.[0]*([0-9]+)$/,"$1.$2.$3.$4");

	return true;
}
/**
 * 检查Form中所有的掩码是否输入了正确的格式
 */
function checkMask(oEle)
{
	var bFlag = false;
	var nMin = parseInt(oEle.getAttribute("min"));
	var nMax = parseInt(oEle.getAttribute("max"));
	
	/*去除前面的0*/
	oEle.value = oEle.value.replace(/^[0]*([0-9]+)\.[0]*([0-9]+)\.[0]*([0-9]+)\.[0]*([0-9]+)$/,"$1.$2.$3.$4");
	var nMask = oEle.value;
	
	var aIPMask = ["0.0.0.0","128.0.0.0", "192.0.0.0", "224.0.0.0", "240.0.0.0", "248.0.0.0", "252.0.0.0",
				   "254.0.0.0", "255.0.0.0", "255.128.0.0", "255.192.0.0", "255.224.0.0", "255.240.0.0",
				   "255.248.0.0", "255.252.0.0", "255.254.0.0", "255.255.0.0", "255.255.128.0","255.255.192.0",
				   "255.255.224.0", "255.255.240.0", "255.255.248.0", "255.255.252.0", "255.255.254.0", "255.255.255.0",
				   "255.255.255.128", "255.255.255.192", "255.255.255.224", "255.255.255.240", "255.255.255.248", 
				   "255.255.255.252", "255.255.255.254", "255.255.255.255"];

	MYUTL_Assert(nMin >= 0 && nMin <=32, "Mask min out of range.");
	MYUTL_Assert(nMax >= 0 && nMax <=32, "Mask max out of range.");
	MYUTL_Assert(nMax >= nMin, "Mask min excess max.");

	for (var i=nMin; i <= nMax; i ++)
	{	
		/*判断是否为掩码长度*/
		if (i  == nMask)
		{
			oEle.value = aIPMask[i] ;
			bFlag = true;
			break;
		}
		/*判断是否为点分式掩码*/
		else if (aIPMask[i] == nMask)
		{
			bFlag = true;
			break;
		}
	}
	/* 掩码输入不正确*/
	if ( false== bFlag)
	{
		ErrorBox(oEle,MYUTL_Sprintf(CTypeRc.RS_ERR_MASK_ERROR,aIPMask[nMin],aIPMask[nMax],nMin,nMax));
		return false;
	}
	return true;
	
}
/**
 * 检查Form中所有的通配符/ 反掩码是否输入了正确的格式
 */
function checkWildcard(oEle)
{
	if ( !validateMask(oEle) )
	{
		ErrorBox(oEle, CTypeRc.RS_ERR_MASK);
		return false;
	}

	oEle.value = oEle.value.replace(/^[0]*([0-9]+)\.[0]*([0-9]+)\.[0]*([0-9]+)\.[0]*([0-9]+)$/,"$1.$2.$3.$4");

	return true;
}

/**
 * 检查Form中所有的IP是否输入了正确的格式,允许为全零的情况。主要在acl使用。
 */
function checkIP4PZ(oEle)
{
	if ( !validateMask(oEle) )
	{
		ErrorBox(oEle, CTypeRc.RS_ERR_IP4PZ);
		return false;
	}

	return true;
}

/**
 * MAC地址检查 
 */
function checkMac(oEle)
{
	if ( oEle.value=="0" )
	{
		oEle.value = "0-0-0";
		return true;
	}

	/**
	 * 有效性校验
	 */
	var IPPattern = /^[0-9a-f]{1,4}-[0-9a-f]{1,4}-[0-9a-f]{1,4}$/i;
	if ( !IPPattern.test(oEle.value) )
	{
		ErrorBox(oEle, CTypeRc.RS_ERR_MAC);
		return false;
	}

	return true;
}

function checkLUMac(oEle)
{
	if ( oEle.value=="0" )
	{
		oEle.value = "0-0-0-0-0-0";
		return true;
	}

	/* 有效性校验 */
	var IPPattern = /^[0-9a-f]{1,2}-[0-9a-f]{1,2}-[0-9a-f]{1,2}-[0-9a-f]{1,2}-[0-9a-f]{1,2}-[0-9a-f]{1,2}$/i;
	if ( !IPPattern.test(oEle.value) )
	{
		ErrorBox(oEle,CTypeRc.RS_ERR_LUMAC);//"MAC address error.");
		return false;
	}

	return true;
}
/**
 * 检查Form中所有的数字输入框中是否输入了正确的格式和范围
 */
function checkNumber(oEle)
{
	/**
	 * 去掉前面所有的"0". 
	 */
	var nNumber = oEle.value;
	if ( /^0+$/.test(nNumber) )
	{
		/**
		 * 如果输入的是"0...0", 则设置为"0"
		 */
		nNumber = "0";
	}
	else
	{
		/**
		 * 去掉非0数字前面所有的"0"
		 */
		nNumber = nNumber.replace(/^0*/, "");
	}

	if ( ! nNumber.isNumber() )
	{
		ErrorBox(oEle, CTypeRc.RS_ERR_NOT_NUMBER);
		return false;
	}

	nNumber = parseInt(nNumber);
	if ( !checkRange(oEle, nNumber) )
	{
		return false;
	}

	oEle.value = nNumber;
	return true;
}

function MakeErrorString(sFormat, sUnSupportedChars)
{
	var s="";
	for(var i=0; i<sUnSupportedChars.length; i++)
	{
		s += sUnSupportedChars.charAt(i) + " ";
	}
	return MYUTL_Sprintf(sFormat, s);
}

/**
 * cType=word的检查函数
 * 不能包含特殊字符和双字节字符
 */
function checkString(oEle)
{
	// 判断输入框中是否有非法字符
	var ch;
	var sValue = oEle.value;

	for(var i=0; i< sValue.length; i++)
	{
		ch = sValue.charAt(i);

		if ( ch<' ' || ch>'~' || -1 != UNSUPPORT_CHAR.indexOf(ch))
		{
			ErrorBox(oEle, MakeErrorString(CTypeRc.RS_UNSUPPORTED_CHAR_EN, UNSUPPORT_CHAR));
			return false;
		}
	}
	
	return checkRange(oEle, sValue.length);
}

/**
 * cType=word的检查函数
 * 不能包含特殊字符, 可以中文
 */
function checkText(oEle)
{
	// 判断输入框中是否有非法字符
	var ch;
	var sValue = oEle.value;
	/**
	 * Web网管不支持的特殊字符列表.
	 */

	for(var i=0; i< sValue.length; i++)
	{
		ch = sValue.charAt(i);

		// 除了Web网管不支持的特殊字符外, 其它的全部支持
		if ( ch<' ' || -1 != UNSUPPORT_CHAR.indexOf(ch))
		{
			ErrorBox(oEle, MakeErrorString(CTypeRc.RS_UNSUPPORTED_CHAR, UNSUPPORT_CHAR));
			return false;
		}
	}

	return checkRange(oEle, sValue.length);
}

/**
 * 检查Form中所有的数字输入框中是否输入了正确的格式和范围
 */
function checkHexStr(oEle)
{
	/**
	 * 只能输入[0-9A-Za-z]的字符
	 */
	sValue = oEle.value;
	if ( /[^0-9a-f]+/i.test(sValue) )
	{
		/**
		 * 只允许输入十六进制字符
		 */
		ErrorBox(oEle, CTypeRc.RS_ERR_ONLY_HEX_CHAR);//"Only [0-9a-zA-Z] allowed.");
		return false;
	}

	return checkRange(oEle, sValue.length);
}

/**
 * 检查是否是一个合法的单词
 */
function checkWord(oEle)
{
	if ( !checkString(oEle) )
	{
		return false;
	}

	/**
	 * 检查空格
	 */
	if ( /[\t\r\n \f\v]/.test(oEle.value) )
	{
		ErrorBox(oEle, CTypeRc.RS_NO_SPACE);
		return false;
	}

	return true;
}

/* 检查是否0-9 a-z A-Z _ - */
function isLimitChar(ch)
{
	if ((ch >= '0') && (ch <='9'))
	{
		return true;
	}

	if ((ch >= 'a') && (ch <= 'z'))
	{
		return true;
	}

	if ((ch >= 'A') && (ch <= 'Z'))
	{
		return true;
	}

	if ((ch == '-') || (ch == '_'))
	{
		return true;
	}

	return false;
}

function CheckLimitStr(oEle)
{
	// 判断输入框中是否有非法字符
	var ch;
	var sValue = oEle.value;

	for(var i=0; i< sValue.length; i++)
	{
		ch = sValue.charAt(i);

		if (! isLimitChar(ch))
		{
			ErrorBox(oEle, CTypeRc.RS_ERR_LIMIT_STR);
			return false;
		}
	}
	
	return checkRange(oEle, sValue.length);
}

function checkPassword(oEle)
{
	/**
	 * 判断输入框中是否有非法字符?
	 */
	var ch;
	var sValue = oEle.value;
	for(var i=0; i< sValue.length; i++)
	{
		ch = sValue.charAt(i);

		if ( ch=='?' )
		{
			ErrorBox(oEle, MYUTL_Sprintf(CTypeRc.RS_INPUT_NOT_ALLOW, ch));
			return false;
		}
	}
	return checkRange(oEle, sValue.length);
}

function validateIP64(IPstr)
{
	try
	{
		if((-1 != IPstr.indexOf(".")) && (0 == IPstr.indexOf("::")))
		{
			return validateIP4(IPstr.split("::")[1]);
		}
		return validateIP6(IPstr);
	}
	catch(e)
	{
		return false;
	}
}

function validateIP6(IPstr)
{
	var array1 = IPstr.split("::");
	if(array1.length == 1)
	{
		var array2 = array1[0].split(":");
		if(array2.length != 8)
		{
			return false;
		}
		return checkipv6sub(array2);
	}
	else if(array1.length ==2)
	{
		var totalLen = 0;
		var result = true;
		if( array1[0] != "" )
		{
			var array2 = array1[0].split(":");
			totalLen += array2.length;
			if(totalLen > 7)
				return false;
			result = checkipv6sub(array2);
		}
		if(array1[1] != "")
		{
			var array2 = array1[1].split(":");
			totalLen += array2.length;
			if(totalLen > 7)
				return false;
			result = result && checkipv6sub(array2);
		}
		return result;
	}
	return false;
}
function checkipv6sub(array)
{
	var IPPattern = /^0*[a-fA-F0-9]{0,4}$/
	for(var i = 0;i<array.length;i++)
	{
		if(array[i] == "")
			return false;
		if(!IPPattern.test(array[i]))
			return false;
	}
	return true;
}

function checkIP6(oEle)
{
	if(!validateIP64(oEle.value))
	{
		ErrorBox(oEle,CTypeRc.RS_ERR_IP6);
		return false;
	}
	return true;
}

function CheckEmail(oEle)
{					   
	var p1 = /^[a-z0-9!#$%&*+-=?^_`{|}~]+(\.[a-z0-9!#$%&*+-=?^_`{|}~]+)*@([-a-z0-9]+\.+.)/i;
	 
	var sValue = oEle.value.toString();
	var illegal = sValue.match(p1);
	
	if (illegal==null)
	{		 
		ErrorBox(oEle, CTypeRc.RS_ERR_EMAIL);
		return false;
	}	
   
	return checkString(oEle, sValue.length);
}

function checkNewNumber(oEle)
{
	var nMin = parseInt(oEle.getAttribute("min"));
	var nMax = parseInt(oEle.getAttribute("max"));
	/**
	 * 去掉前面所有的"0". 2005-6-27 11:51
	 */
	var nNumber = oEle.value;
	if ( /^0+$/.test(nNumber) )
	{
		/**
		 * 如果输入的是"0...0", 则设置为"0"
		 */
		nNumber = "0";
	}
	else
	{
		/**
		 * 去掉非0数字前面所有的"0"
		 */
		nNumber = nNumber.replace(/^0*/, "");
	}

	if ( !nNumber.isNumber() )
	{
		ErrorBox(oEle,MYUTL_Sprintf(CTypeRc.RS_ERR_NOT_IN_INT_RANGE,nMin,nMax));
		return false;
	}

	nNumber = parseInt(nNumber);
	if ( !checkIntRange(oEle, nNumber) )
	{
		return false;
	}
	
	/**
	 * Add by ghfeng --begin
	 */
	var sMultiple = oEle.getAttribute("Multiple");	
	if(null != sMultiple)
	{
		nMultiple = parseInt(sMultiple);
		if ( !checkMultiple(oEle, nNumber, nMultiple) )
		{
			ErrorBox(oEle,MYUTL_Sprintf(CTypeRc.RS_ERR_NOT_MULTIPLE,nMultiple));//"Error number.");
			return false;
		}
	}

	oEle.value = nNumber;
	return true;
}

function checkIntRange(oEle, nValue)
{
	/**
	 * 因为是常量，所以忽略非数字的检查
	 */
	var nMin = parseInt(oEle.getAttribute("min"));
	var nMax = parseInt(oEle.getAttribute("max"));

	if ( (!isNaN(nMin) && nValue<nMin)
		||(!isNaN(nMax) && nValue>nMax)
		)
	{
		ErrorBox(oEle,MYUTL_Sprintf(CTypeRc.RS_ERR_NOT_IN_INT_RANGE,nMin,nMax));
		return false;
	}
	
	return true;
}

/**
 * 检查Form中所有的字符串输入框中是否输入了正确的格式和范围
 * 检查是否为某个整数的倍数
 * nNumber待检查的整数,nMultiple倍数
 */
function checkMultiple(oEle, nNumber, nMultiple)
{
	/**
	 * 因为是常量，所以忽略非数字的检查
	 */
	if ( 0 != (nNumber % nMultiple))
	{
		return false;
	}	
	return true;
}

function checkString_urlfil(oEle)
{
	var nMin = parseInt(oEle.getAttribute("min"));
	var nMax = parseInt(oEle.getAttribute("max"));
	var ch;
	var sValue = oEle.value;
	for(var i=0; i< sValue.length; i++) 
	{
		ch = sValue.charAt(i);
		if ( ch<' ' || ch>'~' || ch=='?' || ch==' ')
		{
			ErrorBox(oEle,MYUTL_Sprintf(CTypeRc.RS_INPUT_NOT_ALLOW_URLFIL,nMin,nMax));
			return false;
		}
	}

	return checkStrRange(oEle, sValue.length);
}

function checkString_name(oEle)
{
	var nMin = parseInt(oEle.getAttribute("min"));
	var nMax = parseInt(oEle.getAttribute("max"));
	var ch;
	var sValue = oEle.value;
	for(var i=0; i< sValue.length; i++) 
	{
		ch = sValue.charAt(i);
		if (ch<' ' || ch>'~' || ch=='?')
		{
			ErrorBox(oEle,MYUTL_Sprintf(CTypeRc.RS_INPUT_NOT_ALLOW_NAME,nMin,nMax));
			return false;
		}
	}

	return checkStrRange(oEle, sValue.length);
}

function checkStrRange(oEle, nValue)
{
	var nMin = parseInt(oEle.getAttribute("min"));
	var nMax = parseInt(oEle.getAttribute("max"));
	var sErr;

	if ( (!isNaN(nMin) && nValue<nMin)
		||(!isNaN(nMax) && nValue>nMax)
		)
	{
		if(nMin == nMax)
		{
			sErr = MYUTL_Sprintf(CTypeRc.RS_ERR_NOT_EQUAL_STR_LENGTH,nMin);//"NOT equal str length.");	 
		}
		else
		{
			sErr = MYUTL_Sprintf(CTypeRc.RS_ERR_NOT_IN_STR_RANGE,nMin,nMax);//"NOT in str range.");
		}
		ErrorBox(oEle,sErr);
		return false;
	}
	
	return true;
}
})();


function CType_CheckForm(oForm)
{
	return CType.checkInputForm(oForm);
}

function CType_CheckElement(oEle)
{
	return CType.checkInputElement(oEle);
}


<!--//--><![CDATA[//><!--
ctype_onload = function()
{
	var sfEls = document.getElementsByTagName("INPUT");
	
	for (var i=0; i<sfEls.length; i++)
	{
		sfEls[i].ctype_onblur = sfEls[i].onblur;

		sfEls[i].onblur = function()
		{
			CType_CheckElement(this);
			this.ctype_onblur();
		}
	}
}

AddEvent("load", ctype_onload);
//--><!]]>


