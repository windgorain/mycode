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

	if (oJson.Status == "Enable")	{
		$("#Enable").prop("checked", true);
	}	
}

function my_build_property()
{
	AdminCheckOnline();

	var sEnable = "";
	if ($('#Enable').prop("checked"))
	{
		sEnable = "1";
	}

	return "&Enable=" + sEnable;
}

/* 特有部分 */
