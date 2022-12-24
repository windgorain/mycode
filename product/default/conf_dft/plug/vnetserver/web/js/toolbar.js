
$(document).ready(function() {
	/* TopBar Begin */
	var sHtml = "<div class=\"TopBar\"><img class=\"Logo\" src=\"/_base/img/logo.gif\" /></div>";
	/* TopBar End */

	/* ToolBar Begin */
	var toolbar = ToolBar_Create();
	toolbar.AddList("首页@'/',下载@/download/download.htm");
	
	var sRight='登录@/user/login.htm, 注册@/user/register.htm';
	
	if (CheckOnline() == true)
	{
		sRight='域管理@/domain/domain.htm, 退出@/user/logout.htm';
	}

	toolbar.AddList(sRight, "right");
	toolbar.SetCurrentName($('#toolbar').attr("title"));
  /* ToolBar End */
  
  sHtml += toolbar.GetHtml();
	
	$('#toolbar').html(sHtml);
});


