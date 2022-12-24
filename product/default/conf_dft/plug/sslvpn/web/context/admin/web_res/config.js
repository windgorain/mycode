var g_JsNoDefs = {
  "Module":"WebRes"
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
	$("#url").val(oJson.URL);
	if (oJson.Permit == "true")
	{
		$("#permit").prop("checked", true);
	}
	else
	{
		$("#permit").prop("checked", false);
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
	
	var sUrl = $('#url').val();
	if (sUrl == null)
	{
		sUrl = "";
	}
	
	var sPermit = "false";
	var bPermit = $("#permit").prop("checked");
	if (true == bPermit)
	{
		sPermit = "true";
	}
	
	return "&Description=" + sDesc + "&URL=" + sUrl + "&Permit=" + sPermit;
}

