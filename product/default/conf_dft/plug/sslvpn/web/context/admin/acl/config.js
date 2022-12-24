var g_JsNoDefs = {
  "Module":"ACL"
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
	$("#Rules").val(oJson.Rules.replaceAll(">", "\n"));
}

function my_build_property()
{
	AdminCheckOnline();
	
	var sDesc = $('#Description').val();
	if (sDesc == null)
	{
		sDesc = "";
	}
	
	var sRules = $('#Rules').val();
	if (sRules == null)
	{
		sRules = "";
	}
	
	sRules = sRules.removeBlankLine();
	sRules = sRules.replace(/[\r\n]/g, ">");
	
	return "&Description=" + sDesc + "&Rules=" + sRules;
}


/* 特有部分 */
function check_rules()
{
	var sRules = $("#Rules").val();
	
	if (sRules.indexOf(">") >= 0)
	{
		return "不能输入'>'";
	}
	
	return null;
}