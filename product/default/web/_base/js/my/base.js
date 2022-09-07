
/* Begin: 将IE和FireFox的attachEvent/addEventListener统一封装 */
function AddEvent(szEvent, pfEventFunc)
{
	if (window.attachEvent)
	{
		window.attachEvent("on" + szEvent, pfEventFunc);
	}
	else if (window.addEventListener)
	{
		window.addEventListener(szEvent, pfEventFunc, false);
	}
}
/* End */



