
/**
 * Desc:
	���FORM�е�һЩ�Զ�������, ֻ��<input type=text>��Ч

 * ֧�ֵ�����
	1. ctype - ÿ��ȡֵ���в�ͬ�����Զ�Ӧ���ֱ����£�
		mask			IPV4��ʽ���ַ���, ��������0.0.0.0
		mac				MAC��ַ�ַ���, ��ʽΪHHHH-HHHH-HHHH. ��������0-0-0
		lumac			MAC��ַ�ַ���, ��ʽΪHH-HH-HH-HH-HH-HH
		ip4				IPV4��ʽ���ַ���, ����������0.0.0.0
		ip4pz			IPV4��ʽ���ַ���, ��������0.0.0.0
		int				ʮ��������, ǰ���ո��0��������, ��֧��0x��ʮ�����Ƹ�ʽ.
		newint			ʮ��������, ǰ���ո��0��������, ��֧��0x��ʮ�����Ƹ�ʽ.��������Multiple="2",������Ƿ�Ϊ2�ı���
		hStr			ʮ�������ַ���, ֻ�����ַ� 0-9 a-f A-F
		word			���ܰ������ĺ�?<>\"%'&#�Ϳո�
		str				���ܰ������ĺ�?<>\"%'&#, �����Ϳ��Բ�д
		text			���ܰ��������κ��ַ�֮һ ? < > \ " % ' & # 
		password
		str_name   		���������ַ�����������
		str_urlfil		���������ַ����������ġ��ո�
		ip6				ipv6 ��ַ
		wildcard 		ͨ���
		custom			�Զ���
		email			E-Mail
		limit_str		ֻ������������ ��ĸ _ -


	2. ErrMsg - �����ַ����� ���ڼ�鲻ͨ��ʱ��ʾ���û��� ���û������ʾĬ����ʾ��

	3. min and max.
		min - ��Сֵ�����û����������Сֵ
		max - ���ֵ�����û�����������ֵ
		����wType��ֵ��ͬ�в�ͬ������
		1). int, ��ʱ��ʾ��ֵ�����ֵ����Сֵ
		2). hStr, word, str, ��ʱ��ʾ�ַ�������󳤶Ⱥ���С����, ���ĳһ�����
			Ϊ��������, ����ͨ��������С����Ϊ����0����ʵ��

 * ����������
	CType_CheckForm
	
 */

var CTypeRc = {};

CTypeRc.RS_ERROR = "����";
CTypeRc.RS_INPUT_NOT_ALLOW_URLFIL = "�����볤����%d��%d��Χ�ڵ��ַ��������Ҳ��ܰ������������ո�������ַ���";
CTypeRc.RS_INPUT_NOT_ALLOW_URL = "�����볤����%d��%d��Χ�ڵ��ַ��������Ҳ��ܰ��������ַ���";
CTypeRc.RS_INPUT_NOT_ALLOW_DOMAIN = "�����볤����%d��%d��Χ�ڵ��ַ��������Ҳ��ܰ�������������\\������/������@��������������*��������������|������<������>���������ַ���";//����msr��wlanģ���domain
CTypeRc.RS_ERR_NOT_MULTIPLE = "ֻ����������0-9��ɵ�ʮ��������������%d�ı�����";
CTypeRc.RS_INPUT_NOT_ALLOW_SPACE = "�����볤����%d��%d��Χ�ڵ��ַ��������Ҳ��ܰ��������������������ո�������ַ���";
CTypeRc.RS_INPUT_NOT_ALLOW_NAME = "�����볤����%d��%d��Χ�ڵ��ַ��������Ҳ��ܰ����������������ַ���";
CTypeRc.RS_INPUT_NOT_ALLOW_PASSWORD = "�����볤����%d��%d��Χ�ڻ�88���ַ��������Ҳ��ܰ����������������ַ���";
CTypeRc.RS_INPUT_NOT_ALLOW_STRING = "�����볤����%d��%d��Χ�ڵ��ַ��������Ҳ��ܰ�����/������\\��������������|������@������*����������������������<������>���������ַ���";

CTypeRc.RS_INPUT_NOT_PVC_TYPE = "������PVC��VPI/VCIֵ�ԣ�VPI��VCIֵ��Ϊ%d��%d��Χ�ڵ����������Ҳ���ͬʱΪ0��";
CTypeRc.RS_INPUT_NOT_TIMESLOT_TYPE = "������ʱ϶����ֵ������1��3-6��ʱ϶����ֵΪ%d��%d��Χ�ڵ�������";
CTypeRc.RS_INPUT_NOT_ALLOW = "�зǷ��ַ���%s�����롣";
CTypeRc.RS_UNSUPPORTED_CHAR_EN = "���ܰ������ĺ������κ��ַ�֮һ\r\n\r\n%s";
CTypeRc.RS_UNSUPPORTED_CHAR = "���ܰ��������κ��ַ�֮һ\r\n\r\n%s";
CTypeRc.RS_NO_SPACE = "����������ո�";
CTypeRc.RS_ERR_NOT_IN_RANGE = "����ֵ����ָ���ķ�Χ�ڡ�";
CTypeRc.RS_ERR_NOT_IN_INT_RANGE = "������%d��%d��Χ�ڵ�������";
CTypeRc.RS_ERR_NOT_IN_STR_RANGE = "�����볤����%d��%d��Χ�ڵ��ַ���";
CTypeRc.RS_ERR_NOT_EQUAL_STR_LENGTH = "�����볤��Ϊ%d���ַ�����";
CTypeRc.RS_ERR_IP4 = "IP��ַ���Ϸ���";
CTypeRc.RS_ERR_IP4PZ = "IP��ַ���Ϸ���";
CTypeRc.RS_ERR_MASK = "���������ϵ��ʮ���Ƶĸ�ʽ��";
CTypeRc.RS_ERR_MASK_ERROR = "��������ʮ���Ƹ�ʽ�����루%s��%s�����������볤�ȣ�%d��%d����";
CTypeRc.RS_ERR_SERIES_MASK_ERROR = "�������������롣";
CTypeRc.RS_ERR_MAC = "MAC��ַ�������ʽӦΪH-H-H��";
CTypeRc.RS_ERR_MAC_STRICT = "��Ч��MAC��ַ��";
CTypeRc.RS_ERR_UNIMAC = "�������ʽΪH-H-H�ĵ���MAC��ַ��";
CTypeRc.RS_ERR_LUMAC = "�������ʽΪHH-HH-HH-HH-HH-HH��MAC��ַ��";
CTypeRc.RS_ERR_NOT_NUMBER = "ֻ��������������";
CTypeRc.RS_ERR_ONLY_HEX_CHAR = "ֻ��������ʮ����������";
CTypeRc.RS_ERR_ONLY_DEC_CHAR = "ֻ����������0-9��ɵ��ַ�����";
CTypeRc.RS_ERR_IP6 = "IPv6��ַ���Ϸ���";
CTypeRc.RS_ERR_NOT_EMPTY = "����������ա�";
CTypeRc.RS_ERR_SELVLAN_FIRST = "����ѡ��Ҫ������VLAN��Χ��";
CTypeRc.RS_ERR_QUOTATION = "�������";
CTypeRc.RS_LOOPBACK_MASK = "LoopBack�˿ڽ�֧��32λ����ģʽ��Ҫ����32λ������";
CTypeRc.RS_ERR_WLANSTR = "ֻ����������0-9��a-z��A-Z����ɵ��ַ�����";
CTypeRc.RS_ERR_COLON = "��";
CTypeRc.RS_ERR_PASSWORD = "����������벻���Կո�ͷ���β��";
CTypeRc.RS_MENU_CWMP = "CWMP";
CTypeRc.RS_MENU_CWMP_DESC = "CWMP";
CTypeRc.RS_TAB_CWMPSUMMARY = "��ʾ";
CTypeRc.RS_TAB_CWMPSUMMARY_DESC = "��ʾ��ǰCWMP����";
CTypeRc.RS_TAB_CWMPSETUP = "����";
CTypeRc.RS_TAB_CWMPSETUP_DESC = "����CWMP����";
CTypeRc.RS_ERR_EMAIL = "�ʼ���ַ��ʽ���Ϸ���";
CTypeRc.RS_ERR_NOT_NEXTHOPEMPTY = "��һ����ַ����Ϊ��"; /* Added by heshan for WLD29602 WLD30732 WLD30562 */
CTypeRc.RS_ERR_NOT_IFNEXTHOPBOTHEMPTY = "���ӿں���һ����ַ����ͬʱΪ��";
CTypeRc.RS_ERR_LIMIT_STR = "ֻ�������������֡���ĸ��_��-��ɵ��ַ���";




var CType;
if (CType && (typeof CType != "object" || CType.NAME))
	throw new Error("Namespace 'CType' already exists");

/**
 * �������ֿռ� : CType
 */
CType = {};

/**
 * ��Ӵ����ֿռ�ĺ�����һЩ��Ϣ
 */
CType.NAME = "CType";  // ����
CType.VERSION = "0.1";	// �汾

/**
 * ���������������ֱ�ӵ��ã���������������Form���ֿռ�
 * ����������
	1. checkInputForm(oForm)
 */
(function(){
// Public:

	/**
	 * �����������⺯����Form���ֿռ�
	 */
	CType.checkInputForm = checkInputForm; 
	CType.checkInputElement = checkInputElement;

// Private:

	/**
	 * ��֧�ֵ������ַ��б�.
	 */
	var UNSUPPORT_CHAR = "?<>\\\"%'&#";

	/**
	 * ���ñհ��ķ�����ʹ��˽�����ݺͺ�������˽�У��������Ӱ�죬Ҳ��Ӱ�����(������ȫ�������ռ�)
	 * ����Լ�����»��߿�ʼ�ı����ͺ�������Լ����ʽ���������������ڲ����ݣ����һ�����ȫ�������ռ�
	 */

	var bAlertBox = false; /* �Ƿ񵯳���ʾ�� */

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
	 * ֻ���input�����
	 * ֻ��enable�������������
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
	 * ��ʼ��顣��������������ɾ��ǰ��ո�
	 * 1�����������������ַ�������Ҫɾ���ո�
	 * 2���ļ���ĳЩ�������ɾ���ո�ʱ����������Ҫ����
	 * 3��wType="text"ʱ��Ϊ�������֧�֣�Ҳ����Ҫɾ���ո�
	 */
	
	sEleType = oEle.type.toLowerCase();
	if(sEleType!="password" && sEleType!="file" && cType!="text")/* "file"Ϊ������������� */
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
 * �������IP��ַ����У��,��ʽΪxx.xx.xx.xx
 */
function validateIP4(IPstr)
{
	/**
	 * ��Ч��У��
	 */
	var IPPattern = /^\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}$/
	if(!IPPattern.test(IPstr))return false;

	/**
	 * �����ֵ
	 */
	var IPArray = IPstr.split(".");
	if(IPArray.length != 4)return false;

	var ip1 = parseInt(IPArray[0],10);
	var ip2 = parseInt(IPArray[1],10);
	var ip3 = parseInt(IPArray[2],10);
	var ip4 = parseInt(IPArray[3],10);
	/**
	 * ÿ����ֵ��Χ0-255
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
 * �������IP��ַ����У��,��ʽΪxx.xx.xx.xx
 */
function validateMask(oEle)
{
	var MaskStr = oEle.value;

	/**
	 * ��Ч��У��
	 */
	var IPPattern = /^\d{1,}\.\d{1,}\.\d{1,}\.\d{1,}$/

	if ( MaskStr == "0" )
	{
		oEle.value = "0.0.0.0";
		return true;
	}
	if(!IPPattern.test(MaskStr))return false;

	/**
	 * �����ֵ
	 */
	var IPArray = MaskStr.split(".");
	var ip1 = parseInt(IPArray[0]);
	var ip2 = parseInt(IPArray[1]);
	var ip3 = parseInt(IPArray[2]);
	var ip4 = parseInt(IPArray[3]);
	/**
	 * ÿ����ֵ��Χ0-255
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
		 * typeΪhidden����£�oEle.focus()���쳣�������޷�����ʾ��Ϣ�������try����
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
	 * ��Ϊ�ǳ��������Ժ��Է����ֵļ��
	 */
	var nMin = parseInt(oEle.getAttribute("min"));
	var nMax = parseInt(oEle.getAttribute("max"));
	/**
	 * �����Сֵ ���ֵ
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
 * �Զ�ȥ��Form������������ǰ��ո�.
 */
function trim(oEle)
{
	oEle.value = oEle.value.trim();
	return true;
}

/**
 * ���Form�����е�IP�Ƿ���������ȷ�ĸ�ʽ
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
 * ���Form�����е������Ƿ���������ȷ�ĸ�ʽ
 */
function checkMask(oEle)
{
	var bFlag = false;
	var nMin = parseInt(oEle.getAttribute("min"));
	var nMax = parseInt(oEle.getAttribute("max"));
	
	/*ȥ��ǰ���0*/
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
		/*�ж��Ƿ�Ϊ���볤��*/
		if (i  == nMask)
		{
			oEle.value = aIPMask[i] ;
			bFlag = true;
			break;
		}
		/*�ж��Ƿ�Ϊ���ʽ����*/
		else if (aIPMask[i] == nMask)
		{
			bFlag = true;
			break;
		}
	}
	/* �������벻��ȷ*/
	if ( false== bFlag)
	{
		ErrorBox(oEle,MYUTL_Sprintf(CTypeRc.RS_ERR_MASK_ERROR,aIPMask[nMin],aIPMask[nMax],nMin,nMax));
		return false;
	}
	return true;
	
}
/**
 * ���Form�����е�ͨ���/ �������Ƿ���������ȷ�ĸ�ʽ
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
 * ���Form�����е�IP�Ƿ���������ȷ�ĸ�ʽ,����Ϊȫ����������Ҫ��aclʹ�á�
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
 * MAC��ַ��� 
 */
function checkMac(oEle)
{
	if ( oEle.value=="0" )
	{
		oEle.value = "0-0-0";
		return true;
	}

	/**
	 * ��Ч��У��
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

	/* ��Ч��У�� */
	var IPPattern = /^[0-9a-f]{1,2}-[0-9a-f]{1,2}-[0-9a-f]{1,2}-[0-9a-f]{1,2}-[0-9a-f]{1,2}-[0-9a-f]{1,2}$/i;
	if ( !IPPattern.test(oEle.value) )
	{
		ErrorBox(oEle,CTypeRc.RS_ERR_LUMAC);//"MAC address error.");
		return false;
	}

	return true;
}
/**
 * ���Form�����е�������������Ƿ���������ȷ�ĸ�ʽ�ͷ�Χ
 */
function checkNumber(oEle)
{
	/**
	 * ȥ��ǰ�����е�"0". 
	 */
	var nNumber = oEle.value;
	if ( /^0+$/.test(nNumber) )
	{
		/**
		 * ����������"0...0", ������Ϊ"0"
		 */
		nNumber = "0";
	}
	else
	{
		/**
		 * ȥ����0����ǰ�����е�"0"
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
 * cType=word�ļ�麯��
 * ���ܰ��������ַ���˫�ֽ��ַ�
 */
function checkString(oEle)
{
	// �ж���������Ƿ��зǷ��ַ�
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
 * cType=word�ļ�麯��
 * ���ܰ��������ַ�, ��������
 */
function checkText(oEle)
{
	// �ж���������Ƿ��зǷ��ַ�
	var ch;
	var sValue = oEle.value;
	/**
	 * Web���ܲ�֧�ֵ������ַ��б�.
	 */

	for(var i=0; i< sValue.length; i++)
	{
		ch = sValue.charAt(i);

		// ����Web���ܲ�֧�ֵ������ַ���, ������ȫ��֧��
		if ( ch<' ' || -1 != UNSUPPORT_CHAR.indexOf(ch))
		{
			ErrorBox(oEle, MakeErrorString(CTypeRc.RS_UNSUPPORTED_CHAR, UNSUPPORT_CHAR));
			return false;
		}
	}

	return checkRange(oEle, sValue.length);
}

/**
 * ���Form�����е�������������Ƿ���������ȷ�ĸ�ʽ�ͷ�Χ
 */
function checkHexStr(oEle)
{
	/**
	 * ֻ������[0-9A-Za-z]���ַ�
	 */
	sValue = oEle.value;
	if ( /[^0-9a-f]+/i.test(sValue) )
	{
		/**
		 * ֻ��������ʮ�������ַ�
		 */
		ErrorBox(oEle, CTypeRc.RS_ERR_ONLY_HEX_CHAR);//"Only [0-9a-zA-Z] allowed.");
		return false;
	}

	return checkRange(oEle, sValue.length);
}

/**
 * ����Ƿ���һ���Ϸ��ĵ���
 */
function checkWord(oEle)
{
	if ( !checkString(oEle) )
	{
		return false;
	}

	/**
	 * ���ո�
	 */
	if ( /[\t\r\n \f\v]/.test(oEle.value) )
	{
		ErrorBox(oEle, CTypeRc.RS_NO_SPACE);
		return false;
	}

	return true;
}

/* ����Ƿ�0-9 a-z A-Z _ - */
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
	// �ж���������Ƿ��зǷ��ַ�
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
	 * �ж���������Ƿ��зǷ��ַ�?
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
	 * ȥ��ǰ�����е�"0". 2005-6-27 11:51
	 */
	var nNumber = oEle.value;
	if ( /^0+$/.test(nNumber) )
	{
		/**
		 * ����������"0...0", ������Ϊ"0"
		 */
		nNumber = "0";
	}
	else
	{
		/**
		 * ȥ����0����ǰ�����е�"0"
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
	 * ��Ϊ�ǳ��������Ժ��Է����ֵļ��
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
 * ���Form�����е��ַ�����������Ƿ���������ȷ�ĸ�ʽ�ͷ�Χ
 * ����Ƿ�Ϊĳ�������ı���
 * nNumber����������,nMultiple����
 */
function checkMultiple(oEle, nNumber, nMultiple)
{
	/**
	 * ��Ϊ�ǳ��������Ժ��Է����ֵļ��
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


