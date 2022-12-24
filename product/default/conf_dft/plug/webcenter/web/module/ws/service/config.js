var g_JsNoDefs = {
  "Module":"ws.service"
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

	if (oJson.Enable == "1")	{
		$("#Enable").prop("checked", true);
	}	
}

function my_build_property()
{
	AdminCheckOnline();
	
	var sDesc = $('#Description').val();
	if (sDesc == null)
	{
		sDesc = "";
	}
	
	var sEnable = "";
	if ($('#Enable').prop("checked"))
	{
		sEnable = "1";
	}

	return "&Description=" + sDesc + "&Enable=" + sEnable;
}

/* 特有部分 */
