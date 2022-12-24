var g_JsNoDefs = {
  "Module":"wan.route"
};

function MyAddInit()
{
	AdminCheckOnline();
}

function MyModifyInit(sName)
{	
	AdminCheckOnline();
}

function my_build_property()
{
	AdminCheckOnline();
	
	var sIP = $('#IP').val();
	if (sIP == null)
	{
		sIP = "0.0.0.0";
	}
	
	var sMask = $('#Mask').val();
	if (sMask == null)
	{
		sMask = "443";
	}
	
	var sNexthop = $('#Nexthop').val();
	if (sNexthop == null)
	{
		sNexthop = "";
	}

	return "&IP=" + sIP + "&Mask=" + sMask + "&Nexthop=" + sNexthop;
}

/* 特有部分 */
