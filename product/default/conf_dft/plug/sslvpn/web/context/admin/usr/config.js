var g_JsNoDefs = {
  "Module":"LocalUser"
};

function MyAddInit()
{
	AdminCheckOnline();
	selector_box_init("role", "/request.cgi?_do=Role.List");
}

function MyModifyInit(sName)
{
	AdminCheckOnline();
	$("#name").val(sName);
	var oJson = get_info(sName);
	selector_box_init("role", "/request.cgi?_do=Role.List",oJson.Role);
}

function my_build_property()
{
	AdminCheckOnline();
	
	var sRoles = $('#role').val();
	if (sRoles == null)
	{
		sRoles = "";
	}
	
	return "&Password=" + $("#password").val() + "&Role=" + sRoles;
}

/* 特有部分 */
function compare_password()
{
	if ($("#password").val() != $("#password2").val())
	{
		return "密码不一致";
	}

	return null;
}