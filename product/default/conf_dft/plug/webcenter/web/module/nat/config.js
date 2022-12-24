var g_JsNoDefs = {
  "Module":"wan.nat"
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
	
	var sInterface = $('#Interface').val();
	if (sInterface == null)
	{
		sInterface = "";
	}
	
	return "&Interface=" + sInterface;
}

/* 特有部分 */
