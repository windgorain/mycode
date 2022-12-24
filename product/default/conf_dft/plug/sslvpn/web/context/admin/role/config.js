var g_JsNoDefs = {
  "Module":"Role"
};

function MyAddInit()
{
	AdminCheckOnline();
	
	selector_box_init("ACL", "/request.cgi?_do=ACL.List");
	selector_box_init("WebRes","/request.cgi?_do=WebRes.List");
	selector_box_init("TcpRes","/request.cgi?_do=TcpRes.List");
	selector_box_init("IpRes","/request.cgi?_do=IpRes.List");
}

function MyModifyInit(sName)
{
	AdminCheckOnline();
	
	$("#name").val(sName);
	var oJson = get_info(sName);
	$("#Description").val(oJson.Description);
	selector_box_init("ACL", "/request.cgi?_do=ACL.List",oJson.ACL);
	selector_box_init("WebRes","/request.cgi?_do=WebRes.List", oJson.WebRes);
	selector_box_init("TcpRes","/request.cgi?_do=TcpRes.List", oJson.TcpRes);
	selector_box_init("IpRes","/request.cgi?_do=IpRes.List", oJson.IpRes);
}

function my_build_property()
{
	AdminCheckOnline();
	
	var sDesc = $('#Description').val();
	if (sDesc == null)
	{
		sDesc = "";
	}
	
	var sAcl = $('#ACL').val();
	if (sAcl == null)
	{
		sAcl = "";
	}
	
	var sWebRes = $('#WebRes').val();
	if (sWebRes == null)
	{
		sWebRes = "";
	}
	
	var sTcpRes = $('#TcpRes').val();
	if (sTcpRes == null)
	{
		sTcpRes = "";
	}
	
	var sIpRes = $('#IpRes').val();
	if (sIpRes == null)
	{
		sIpRes = "";
	}
	
	return "&Description=" + sDesc
					 + "&ACL=" + sAcl
					 + "&WebRes=" + sWebRes
					 + "&TcpRes=" + sTcpRes
					 + "&IpRes=" + sIpRes;
}
