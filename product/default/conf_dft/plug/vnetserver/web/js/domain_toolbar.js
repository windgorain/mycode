
$(document).ready(function() {
	/* TopBar Begin */
	var sHtml = "<div class=\"TopBar\"><img class=\"Logo\" src=\"/_base/img/logo.gif\" /></div>";
	/* TopBar End */

	/* ToolBar Begin */
	var toolbar = ToolBar_Create();
	toolbar.AddList("首页@'/' target=_parent,下载@/download/download.htm target=_parent");
	
	sRight='域管理@/domain/domain.htm target=_parent, 退出@/user/logout.htm target=_parent';

	toolbar.AddList(sRight, "right");
	toolbar.SetCurrentName($('#toolbar').attr("title"));
  /* ToolBar End */
  
  sHtml += toolbar.GetHtml();
	
	$('#toolbar').html(sHtml);
});


