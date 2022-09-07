/* 公开部分 */
function JS_NO_IsExist(oEle)
{
	my_is_exist(oEle);
}

/* 成功返回true, 失败返回false */
function JS_NO_Add()
{
	return js_no_submit(g_JsNoDefs.Module + ".Add");
}

/* 成功返回true, 失败返回false */
function JS_NO_Modify()
{
	return js_no_submit(g_JsNoDefs.Module + ".Modify");
}

function JS_NO_AddAndGoList()
{
	if (JS_NO_Add()) {JS_NO_GoList();}
}

function JS_NO_ModifyAndGoList()
{
	if (JS_NO_Modify()) {JS_NO_GoList();}
}

function JS_NO_GoList()
{
	window.location = "list.htm";
}

/*不公开部分*/
function js_no_submit(sAction)
{
	var oForm = document.getElementById("MyForm");
	if (CType_CheckForm(oForm) == false)
	{
		return false;
	}
	
	var sRequest = "_do=" + sAction + "&Name=" + $("#name").val() + my_build_property();

	var bResult = false;

	$.ajax(
		{ type: "POST",
			url: "/request.cgi",
			data: sRequest,
			async: false,
			dataType: "json",
			success: function(oJson) {
				if(RQ_IsOK(oJson)) {
					alert("结果: 失败\r\n原因: " + oJson.reason);
				} else {
					bResult = true;
				}
			}
		});

		return bResult;
}	

function get_info(sName)
{
	var sRet = null;

	$.ajax(
		{ type: "GET",
			url: "/request.cgi?_do=" + g_JsNoDefs.Module + ".Get&Name=" + sName,
			dataType: "json",
			async: false,
			success: function(oJson) {
				if(RQ_IsOK(oJson)) {
					sRet = oJson;
				}
			}
		});

	return sRet;
}

function my_is_exist(oEle)
{  
	var sName=oEle.value;
	var sn = oEle._ctype_sn;

	CType_SetChecking(oEle, "正在检查是否存在...");
		
	$.ajax(
		{ type: "POST",
			url: "/request.cgi",   
			data: "_do=" + g_JsNoDefs.Module + ".IsExist&Name=" + sName,
			async: true,
			dataType: "json",
			success: function(oJson) {
				if (sn != oEle._ctype_sn)
				{
					return;
				}
				
				if(RQ_IsOK(oJson)) {
					if (oJson.exist == "True") {
						CType_SetErrorInfo(oEle, "已存在");
					} else {
						CType_SetCheckOk(oEle);
					}
				} else {
					var sErrinfo = "检查失败";
					if (oJson.reason) {
						sErrinfo += ",原因:";
						sErrinfo += oJson.reason;
					}
					CType_SetErrorInfo(oEle, sErrinfo);
				}
			}
		});
}
