var g_JsNoDefs = {
  "Module":"ws.gateway"
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
	$("#Port").val(oJson.Port);
	$("#IP").val(oJson.IP);

	if (oJson.Type == "TCP") {
		$("input:radio[name=Type]").eq(0).prop("checked", true);
	} else {
		$("input:radio[name=Type]").eq(1).prop("checked", true);
	}
	
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

	var sType = $('input:radio[name=Type]:checked').val();
	if (sType == null)
	{
		sType = "";
	}
	
	var sPort = $('#Port').val();
	if (sPort == null)
	{
		sPort = "443";
	}
	
	var sIP = $('#IP').val();
	if (sIP == null)
	{
		sIP = "";
	}
	
	var sEnable = "";
	if ($('#Enable').prop("checked"))
	{
		sEnable = "1";
	}

	return "&Description=" + sDesc + "&Type=" + sType + "&IP=" + sIP + "&Port=" + sPort + "&Enable=" + sEnable;
}

/* 特有部分 */
