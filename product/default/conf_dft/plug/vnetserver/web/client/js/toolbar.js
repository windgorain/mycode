
function user_state()
{
	parent.frames("content").location="/client/user_state.htm";
}

function user_online_list()
{
	parent.frames("content").location="/client/onlineuser.htm";
}

function Hide()
{
	HtmlerCmd('Hide');
	toolbar.SetCurrentName(toolbar.GetOldName());
}

function OnExit()
{
	HtmlerCmd('HtmlerCancel');
}

var toolbar = ToolBar_Create();

$(document).ready(function() {
	/* ToolBar Begin */
	toolbar.Add("状态", user_state);
	toolbar.Add("在线用户", user_online_list);
	toolbar.Add("隐藏", Hide);
	toolbar.Add("退出", OnExit);
	toolbar.BindContainer("toolbar");
	toolbar.ClickNode("状态");
  /* ToolBar End */
});


