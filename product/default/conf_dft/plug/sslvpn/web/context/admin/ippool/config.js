var g_JsNoDefs = {
  "Module":"IPPool"
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
	$("#StartIP").val(oJson.StartIP);
	$("#EndIP").val(oJson.EndIP);
}

function my_build_property()
{
	AdminCheckOnline();
	
	var sDesc = $('#Description').val();
	if (sDesc == null)
	{
		sDesc = "";
	}
	
	var sStartIP = $('#StartIP').val();
	if (sStartIP == null)
	{
		sStartIP = "";
	}
	
	var sEndIP = $('#EndIP').val();
	if (sEndIP == null)
	{
		sEndIP = "";
	}

	return "&Description=" + sDesc + "&StartIP=" + sStartIP + "&EndIP=" + sEndIP;
}

