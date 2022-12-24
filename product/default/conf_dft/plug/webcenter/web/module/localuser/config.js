var g_JsNoDefs = {
  "Module":"localuser"
};

function MyAddInit()
{
	AdminCheckOnline();
}

function MyModifyInit(sName)
{	
	AdminCheckOnline();
	
	$("#name").val(sName);
	var oJson = get_info(sName);
	$("#Description").val(oJson.Description);
}

function my_build_property()
{
	AdminCheckOnline();
	
	var sRet = "";
	
	var sDesc = $('#Description').val();
	if (sDesc == null)
	{
		sDesc = "";
	}
	
	sRet = "&Description=" + sDesc;
	
	var sPassword = $('#password1').val();
	if (sPassword == null)
	{
		sPassword = "";
	}
	
	if (sPassword != "")
	{
		sRet += "&Password=" + sPassword;
	}

	return sRet;
}

/* 特有部分 */
function compare_password()
{
	if ($("#password1").val() != $("#password2").val())
	{
		return "密码不一致";
	}

	return null;
}