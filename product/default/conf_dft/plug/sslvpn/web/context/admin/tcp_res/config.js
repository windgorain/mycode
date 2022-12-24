var g_JsNoDefs = {
  "Module":"TcpRes"
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
	$("#ServerAddress").val(oJson.ServerAddress);
}

function my_build_property()
{
	AdminCheckOnline();
	
	var sDesc = $('#Description').val();
	if (sDesc == null)
	{
		sDesc = "";
	}

	var sServerAddress = $('#ServerAddress').val();
	if (sServerAddress == null)
	{
		sServerAddress = "";
	}
	
	return "&Description=" + sDesc
				 + "&ServerAddress=" + sServerAddress;
}

