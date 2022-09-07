
/*
支持的属性:
    1. ctype:
        none        无CType类型的检查,最宽条件的类型
        text        不能包含下列任何字符之一 ? < > \ " % ' & # 
        int         十进制整数, 前导空格和0将被忽略, 不支持0x的十六进制格式.
        str				  不能包含中文和?<>\"%'&#
        limit_str   数字、字母、下划线、中杠线
        email       Emain地址
        ipv4        ipv4地址,不允许输入全0
        ipv4z       ipv4地址,允许输入全0
        mask        掩码, min和max表示prefixLen允许的范围
        mac         MAC地址.
        
    2.  min, max.
        min - 最小值，如果没有则不限制最小值
        max - 最大值，如果没有则不限制最大值    
        其意义由rangeType 控制, rangeType:
            "number" :  表示数字的范围
            "length" :  表示输入的内容的长度范围
            "mask"   :  掩码长度范围

    3. check回调函数
        ctype_syn_check  - 同步回调函数
        ctype_asyn_check - 异步回调函数

    4. ctype_must
        0 -  默认值, 表示不是必输项.
        1 -  必输项
		
    5. ctype_info_label: 提示信息所用元素的ID
    
    6. ctype_user_tip: 用户的tip信息
		
*/


/* 定义一个CType类 */
function CType_Init()
{
    var obj = new Object();

    /* Public */
    obj.ShowTip = ShowTip;
    obj.CheckForm = CheckForm;
    obj.CheckGlobe = CheckGlobe;
    obj.CheckInput = CheckInput;
    obj.ErrorInfo = ErrorInfo;
    obj.SetChecking = SetChecking;
    obj.SetCheckOk = SetCheckOk;
    obj.IncCtypeSn = IncCtypeSn;

    /* Private: 检查表 */
    var aCtypes = [];
    var oTypeObject;

    oTypeObject = new Object();
    oTypeObject.tip = null;
    oTypeObject.errinfo = null;
    oTypeObject.Check = null;
    oTypeObject.rangeType = "length";
    aCtypes["none"] = oTypeObject;

    oTypeObject = new Object();
    oTypeObject.tip = "数字";
    oTypeObject.errinfo = "只允许输入数字";
    oTypeObject.Check = checkNumber;
    oTypeObject.rangeType = "number";
    aCtypes["int"] = oTypeObject;

    oTypeObject = new Object();
    oTypeObject.tip = null;
    oTypeObject.errinfo = "不能包含下列任何字符之一 ? < > \\ \" % ' & # ";
    oTypeObject.Check = checkText;
    oTypeObject.rangeType = "length";
    aCtypes["text"] = oTypeObject;
    
    oTypeObject = new Object();
    oTypeObject.tip = "不能包含中文和 ? < > \\ \" % ' & #";
    oTypeObject.errinfo = "不能包含中文和 ? < > \\ \" % ' & #";
    oTypeObject.Check = CheckString;
    oTypeObject.rangeType = "length";
    aCtypes["str"] = oTypeObject;

    oTypeObject = new Object();
    oTypeObject.tip = "数字、字母、下划线、中杠线";
    oTypeObject.errinfo = "只允许输入数字、字母、下划线、中杠线";
    oTypeObject.Check = CheckLimitStr;
    oTypeObject.rangeType = "length";
    aCtypes["limit_str"] = oTypeObject;

    oTypeObject = new Object();
    oTypeObject.tip = "e-mail";
    oTypeObject.errinfo = "格式不合法";
    oTypeObject.Check = CheckEmail;
    oTypeObject.rangeType = "length";
    aCtypes["email"] = oTypeObject;

    oTypeObject = new Object();
    oTypeObject.tip = "IP地址";
    oTypeObject.errinfo = "格式不合法";
    oTypeObject.Check = CheckIPv4;
    oTypeObject.rangeType = "length";
    aCtypes["ipv4"] = oTypeObject;

    oTypeObject = new Object();
    oTypeObject.tip = "IP地址";
    oTypeObject.errinfo = "格式不合法";
    oTypeObject.Check = checkIPv4WithZero;
    oTypeObject.rangeType = "length";
    aCtypes["ipv4z"] = oTypeObject;

    oTypeObject = new Object();
    oTypeObject.tip = "IP掩码";
    oTypeObject.errinfo = "格式不合法";
    oTypeObject.Check = checkMask;
    oTypeObject.rangeType = "mask";
    aCtypes["mask"] = oTypeObject;

    oTypeObject = new Object();
    oTypeObject.tip = "MAC地址";
    oTypeObject.errinfo = "格式不合法";
    oTypeObject.Check = checkMac;
    oTypeObject.rangeType = "length";
    oTypeObject.eg = "00:FF:7E:0E:22:03";
    aCtypes["mac"] = oTypeObject;

    /* Private: 变量 */
    var UNSUPPORT_CHAR = "?<>\\\"%'&#";

    /* Public接口 */
    function ShowTip(oEle)
    {
        if (false == NeedCtypeRun(oEle))
        {
            return;
        }

        var cType = get_ctype(oEle);
        var sCtypeInfo = oEle.getAttribute("ctype_info_label");

        if (sCtypeInfo)
        {
            var oCtypeInfo = document.getElementById(sCtypeInfo);

            MYUTL_Assert(oCtypeInfo, "There is not element named " + sCtypeInfo);
            
            if (oCtypeInfo)
            {
                var rangeTip = getRangeTip(oEle);
               	var tip = aCtypes[cType].tip;
                var userTip = getUserTip(oEle);
                var eg = aCtypes[cType].eg;
                var sMsg = "";

                if (tip)
                {
                    sMsg = tip;
                }

                if (rangeTip)
                {
                    sMsg = MYUTL_AddString(sMsg, rangeTip, ",");
                }
                
                if (userTip)
                {
                	sMsg = MYUTL_AddString(sMsg, userTip, ",");
                }

                if (eg)
                {
                    sMsg = MYUTL_AddString(sMsg, "例如:" + eg, ",");
                }

                oCtypeInfo.className = "input_tip";
                oCtypeInfo.innerHTML = sMsg;
            }
        }
    }

    function CheckForm(oForm)
    {
        var oInputs = oForm.getElementsByTagName ("input");
        for ( var i=0; i<oInputs.length; i++ )
        {
            if(!CheckInput(oInputs[i]))
                return false;
        }
        
        oAreas = oForm.getElementsByTagName ("textarea");
        for ( var i=0; i<oAreas.length; i++ )
        {
            if(!CheckInput(oAreas[i]))
                return false;
        }

        if (false == ExtSynCheck(oForm))
        {
            return false;
        }
        
        return true;
    }

    function CheckGlobe(oForm)
    {
        ExtSynCheck(oForm);
    }

    function CheckInput(oEle)
    {
        if (false == NeedCtypeRun(oEle))
        {
            return true;
        }

        TrimInput(oEle);
        
        if (oEle.value == "")
        {
            if (oEle.getAttribute("ctype_must") == 1)
            {
            		ErrorInfo(oEle, "不允许输入空");
                return false;
            }
        }
        else
        {
	        var cType = get_ctype(oEle);

				  if (aCtypes[cType].Check)
	        {
	            if (false == aCtypes[cType].Check(oEle))
	            {
	                ErrorInfo(oEle, aCtypes[cType].errinfo);
	                return false;
	            }
	        }

	        if (false == CheckRange(oEle))
	        {
	            return false;
	        }
				}

        if (false == ExtSynCheck(oEle))
        {
            return false;
        }

        if (false == ExtAsynCheck(oEle))
        {
            return true;
        }

        SetCheckOk(oEle);

        return true;
    }

    function SetCheckOk(oEle)
    {
        var sCtypeInfo = oEle.getAttribute("ctype_info_label");

        if (sCtypeInfo)
        {
            var oCtypeInfo = document.getElementById(sCtypeInfo);
            if (oCtypeInfo)
            {
                oCtypeInfo.className = "input_ok";
                oCtypeInfo.innerHTML = "";
            }
        }
    }

    function SetChecking(oEle, sMsg)
    {
        var sCtypeInfo = oEle.getAttribute("ctype_info_label");

        if (sCtypeInfo)
        {
            var oCtypeInfo = document.getElementById(sCtypeInfo);
            if (oCtypeInfo)
            {
                oCtypeInfo.className = "input_checking";
                oCtypeInfo.innerHTML = sMsg;
            }
        }
    }

    function ErrorInfo(oEle, sMsg)
    {
        var sCtypeInfo = oEle.getAttribute("ctype_info_label");

        if (sMsg == null)
        {
            sMsg = "";
        }

        if (sCtypeInfo)
        {
            var oCtypeInfo = document.getElementById(sCtypeInfo);
            if (oCtypeInfo)
            {
                if (sMsg == "")
                {
                    oCtypeInfo.className = "";
                }
                else
                {
                    oCtypeInfo.className = "input_err";
                }
                oCtypeInfo.innerHTML = sMsg;
            }
        }
    }

    /* Private: 基本函数 */
    function get_ctype(oEle)
    {
    	var cType = oEle.getAttribute("ctype");
    	if (cType == null)
    	{
    		cType = "none";
    	}
    	
    	return cType;
    }
    
    function NeedCtypeRun(oEle)
    {
        /**
        * 只检查input/textarea输入框
        * 只对enable的输入框作处理
        */
        if (oEle.disabled)
        {
        	return false;
        }

        if ((oEle.tagName.toLowerCase() != "input") && (oEle.tagName.toLowerCase() != "textarea"))
        {
        	return false;
        }
        
        if ((oEle.tagName.toLowerCase() == "input") && (oEle.type == undefined))
        {
            return false;
        }

        return true;
    }

    function TrimInput(oEle)
    {
        var sEleType = oEle.type.toLowerCase();

        /**
        * 开始检查。对特殊的情况不能删除前后空格：
        * 1。密码可以输入各种字符，不需要删除空格
        * 2。文件在某些浏览器上删除空格时会出错，因此需要跳过
        * 3。wType="text"或"none"时，也不需要删除空格
        */
        if((sEleType!="password") && (sEleType!="file") && (get_ctype(oEle)!="text") && (get_ctype(oEle)!="none"))
        {
            oEle.value = oEle.value.trim();
        }
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

    function ExtSynCheck(oEle)
    {
    	  if (! oEle)
    	  {
    	  	return true;
    	  }
    	  
        var pfCtypeCheck = oEle.getAttribute("ctype_syn_check");
        if (pfCtypeCheck)
        {
            pfCtypeCheck = pfCtypeCheck.replace("this",'oEle');

            /* 同步check函数, 返回null表示成功,否则返回错误信息 */
            var errinfo = eval(pfCtypeCheck);

            ErrorInfo(oEle, errinfo);

            if ((errinfo) && (errinfo != ""))
            {
                return false;
            }
        }

        return true;
    }

    /* true - 继续向下走. false - 正在执行异步函数 */
    function ExtAsynCheck(oEle)
    {
        var pfCtypeCheck = oEle.getAttribute("ctype_asyn_check");
        if (! pfCtypeCheck)
        {
            return true;
        }
        pfCtypeCheck = pfCtypeCheck.replace("this",'oEle');

        eval(pfCtypeCheck);

        return false;
    }

    function IncCtypeSn(oEle)
    {
        if (! oEle._ctype_sn)
        {
            oEle._ctype_sn = 1;
        }
        else
        {
            oEle._ctype_sn = oEle._ctype_sn + 1;
        }
    }

    function CheckRange(oEle)
    {
        var sMin = oEle.getAttribute("min");
        var sMax = oEle.getAttribute("max");

        if ((! sMin) && (! sMax))
        {
            return true;
        }

        var nMin = parseInt(sMin);
        var nMax = parseInt(sMax);
        var nValue = 0;

        var cType = get_ctype(oEle);
        var rangeType = aCtypes[cType].rangeType;
        
        if (rangeType == "number")
        {
            nValue = parseInt(oEle.value);
        }
        else if (rangeType == "length")
        {
            nValue = oEle.value.length;
        }
        else if (rangeType == "mask")
        {
            nValue = Mask2PrefixLen(oEle);
            if (isNaN(nMin))
            {
                nMin = 0;
            }
            if (isNaN(nMax) || (nMax > 32))
            {
                nMax = 32;
            }
        }
        else
        {
            return true;    /* 不知如何检查范围, 则不做范围检查 */
        }
        
        if ( (!isNaN(nMin) && nValue<nMin)
            ||(!isNaN(nMax) && nValue>nMax))
        {
            ErrorInfo(oEle, "输入值不在指定的范围内");
            return false;
        }

        return true;
    }
    
    function getUserTip(oEle)
    {
    	var sUserTip = oEle.getAttribute("ctype_user_tip");
    	if ((sUserTip == null) || (sUserTip == ""))
    	{
    		return null;
    	}
    	
    	return sUserTip;
    }

    /* 根据Range生成提示信息 */
    function getRangeTip(oEle)
    {
        var sMin = oEle.getAttribute("min");
        var sMax = oEle.getAttribute("max");
        var cType = get_ctype(oEle);
        var rangeType = aCtypes[cType].rangeType;
        var sTip = null;

        if ((sMin) && (sMax))
        {
            if (rangeType == "number")
            {
                sTip = sMin + "-" + sMax;
            }
            else if (rangeType == "length")
            {
                sTip = sMin + "-" + sMax + "个字符";
            }
            else if (rangeType == "mask")
            {
                sTip = sMin + "-" + sMax + "位掩码";
            }
        }
        else if (sMin)
        {
            if (rangeType == "number")
            {
                sTip = "不小于" + sMin;
            }
            else if (rangeType == "length")
            {
                sTip = "不少于" + sMin + "个字符"
            }
            else if (rangeType == "mask")
            {
                sTip = sMin + "-32位掩码";
            }
        }
        else if (sMax)
        {
            if (rangeType == "number")
            {
                sTip = "不大于" + sMax;
            }
            else if (rangeType == "length")
            {
                sTip = "不多于" + sMax + "个字符";
            }
            else if (rangeType == "mask")
            {
                sTip = "0-" + sMax + "位掩码";
            }
        }

        return sTip;
    }

    /* Private: Check系列函数 */
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
				return false;
			}
			
			oEle.value = nNumber;

			return true;
		}
		
		/**
		 * 不能包含特殊字符, 可以中文
		 */
		function checkText(oEle)
		{
			// 判断输入框中是否有非法字符
			var ch;
			var sValue = oEle.value;

			for(var i=0; i< sValue.length; i++)
			{
				ch = sValue.charAt(i);
		
				// 除了Web网管不支持的特殊字符外, 其它的全部支持
				if ( ch<' ' || -1 != UNSUPPORT_CHAR.indexOf(ch))
				{
					return false;
				}
			}

			return true;
		}

		/**
     * 不能包含特殊字符和双字节字符
     */
    function CheckString(oEle)
    {
        // 判断输入框中是否有非法字符
        var ch;
        var sValue = oEle.value;

        for(var i=0; i< sValue.length; i++)
        {
            ch = sValue.charAt(i);

            if ( ch<' ' || ch>'~' || -1 != UNSUPPORT_CHAR.indexOf(ch))
            {
                return false;
            }
        }
        
        return true;
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
                return false;
            }
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
            return false;
        }   
       
        return CheckString(oEle);
    }

    function CheckIPv4(oEle)
    {
        var IPstr = oEle.value;
        if (! IP_ValidateIP4(IPstr) )
        {
            return false;
        }

        /**
         *  for 0xx.0xx.0xx.0xx
         */
        oEle.value = IPstr.replace(/^[0]*([0-9]+)\.[0]*([0-9]+)\.[0]*([0-9]+)\.[0]*([0-9]+)$/,"$1.$2.$3.$4");

        return true;
    }

    /**
     * 检查Form中所有的IP是否输入了正确的格式,允许为全零的情况
     */
    function checkIPv4WithZero(oEle)
    {
        if (oEle.value == "0" )
        {
            oEle.value = "0.0.0.0";
            return true;
        }
                
        if (! IP_ValidateIP4(oEle.value) )
        {
            return false;
        }

        return true;
    }

    /* 比如255.255.0.0, 则返回16表示16位掩码. 返回33表示出错 */
    function Mask2PrefixLen(oEle)
    {
        /*去除前面的0*/
        oEle.value = oEle.value.replace(/^[0]*([0-9]+)\.[0]*([0-9]+)\.[0]*([0-9]+)\.[0]*([0-9]+)$/,"$1.$2.$3.$4");
        
        var aIPMask = ["0.0.0.0","128.0.0.0", "192.0.0.0", "224.0.0.0", "240.0.0.0", "248.0.0.0", "252.0.0.0",
                       "254.0.0.0", "255.0.0.0", "255.128.0.0", "255.192.0.0", "255.224.0.0", "255.240.0.0",
                       "255.248.0.0", "255.252.0.0", "255.254.0.0", "255.255.0.0", "255.255.128.0","255.255.192.0",
                       "255.255.224.0", "255.255.240.0", "255.255.248.0", "255.255.252.0", "255.255.254.0", "255.255.255.0",
                       "255.255.255.128", "255.255.255.192", "255.255.255.224", "255.255.255.240", "255.255.255.248", 
                       "255.255.255.252", "255.255.255.254", "255.255.255.255"];

        for (var i=0; i <= 32; i ++)
        {   
            /*判断是否为掩码长度*/
            if (i == oEle.value)
            {
                oEle.value = aIPMask[i] ;
                return i;
            }
            /*判断是否为点分式掩码*/
            else if (aIPMask[i] == oEle.value)
            {
                return i;
            }
        }

        return 33;
    }

    function checkMask(oEle)
    {
        var nPrefixLen = Mask2PrefixLen(oEle);
        if (nPrefixLen > 32)
        {
            return false;
        }

        return true;
    }

    function checkMac(oEle)
    {
        if ( oEle.value=="0" )
        {
            oEle.value = "0-0-0-0-0-0";
            return true;
        }

        /* 有效性校验 */
        var Pattern = /^[0-9a-f]{1,2}:[0-9a-f]{1,2}:[0-9a-f]{1,2}:[0-9a-f]{1,2}:[0-9a-f]{1,2}:[0-9a-f]{1,2}$/i;
        var Pattern2 = /^[0-9a-f]{1,2}-[0-9a-f]{1,2}-[0-9a-f]{1,2}-[0-9a-f]{1,2}-[0-9a-f]{1,2}-[0-9a-f]{1,2}$/i;
        if (( !Pattern.test(oEle.value) ) && ( !Pattern2.test(oEle.value) ))
        {
            return false;
        }

        oEle.value = oEle.value.replaceAll("-", ":");

        return true;
    }

    return obj;
}


var g_CtypeObject = CType_Init();

function CType_CheckForm(oForm)
{
    if (false == g_CtypeObject.CheckForm(oForm))
    {
        alert("请正确填充表单");
        return false;
    }

    return true;
}

function CType_CheckElement(oEle)
{
    if (false ==  g_CtypeObject.CheckInput(oEle))
    {
        return false;
    }

    /* 同时触发globe检查 */
    return g_CtypeObject.CheckGlobe($(oEle).parents("form").get(0));
}

function CType_ShowTip(oEle)
{
    g_CtypeObject.IncCtypeSn(oEle);
    g_CtypeObject.ShowTip(oEle);
}

function CType_SetChecking(oEle, sMsg)
{
    g_CtypeObject.SetChecking(oEle, sMsg);
}

function CType_SetErrorInfo(oEle, sMsg)
{
    g_CtypeObject.ErrorInfo(oEle, sMsg);
}

function CType_SetCheckOk(oEle)
{
    g_CtypeObject.SetCheckOk(oEle);
}

<!--//--><![CDATA[//><!--
function ctype_init_inputs_onfocus(sfEls)
{
	for (var i=0; i<sfEls.length; i++)
  {
      sfEls[i].ctype_onfocus = sfEls[i].onfocus;
      sfEls[i].onfocus = function()
      {
          CType_ShowTip(this);
					
          if (this.ctype_onfocus)
          {
          	return this.ctype_onfocus();
          }
      }

      sfEls[i].ctype_onblur = sfEls[i].onblur;
      sfEls[i].onblur = function()
      {
          CType_CheckElement(this);
          if (this.ctype_onblur)
          {
          	return this.ctype_onblur();
          }
      }
  }
}
ctype_onload = function()
{
    ctype_init_inputs_onfocus(document.getElementsByTagName("INPUT"));
    ctype_init_inputs_onfocus(document.getElementsByTagName("textarea"));
}

AddEvent("load", ctype_onload);
//--><!]]>

