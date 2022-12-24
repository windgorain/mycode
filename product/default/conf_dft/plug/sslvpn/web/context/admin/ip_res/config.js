var g_JsNoDefs = {
  "Module":"IpRes"
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
	$("#Address").val(oJson.Address);
}

function my_build_property()
{
	AdminCheckOnline();
	
	var sDesc = $('#Description').val();
	if (sDesc == null)
	{
		sDesc = "";
	}
	
	var sAddress = $('#Address').val();
	if (sAddress == null)
	{
		sAddress = "";
	}

	return "&Description=" + sDesc
				 + "&Address=" + sAddress;
}

