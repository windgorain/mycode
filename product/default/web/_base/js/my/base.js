
/* Begin: ��IE��FireFox��attachEvent/addEventListenerͳһ��װ */
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



