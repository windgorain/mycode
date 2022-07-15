
function _init()
{
	HtmlerCmd('SetSize', 'W=200,H=130');
	HtmlerCmd('SetTitle', "Title=SSLVPN");
	HtmlerCmd('CreateTray', "Tip=SSLVPN");
	HtmlerCmd('AddMenuItem', "Menu=文件,Item=隐藏,DoAction=Action=Hide");
	HtmlerCmd('AddMenuItem', "Menu=文件,Item=退出,DoAction=" + HtmlerEncode("Action=HtmlerOK"));

	HtmlerCmd('CreateThread', "DoAction=" + HtmlerEncode("Action=DllCall,File=scm.dll,Func=PlugRun"));
	HtmlerCmd('SetExitAction', "DoAction=" + HtmlerEncode("Action=DllCall,File=scm.dll,Func=PlugStop"));
}

$(document).ready(function() {
	_init();
});