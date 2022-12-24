
/* Online User 工具 */
function AdminIsOnline()
{
	var bOnline = false;
	
	$.ajax(
		{ type: "GET",
			url: "/checkonline.cgi",
			async: false,
			dataType: "json",
			success: function(oJson) {
                if (RQ_IsOK(oJson)) {
					bOnline = true;
				}
			}
		});
		
		return bOnline;
}

function AdminCheckOnline()
{
	if (AdminIsOnline())
	{
		return true;
	}
	
	window.alert("请重新登录");
	window.parent.location = "/index.htm";
	
	return false;
}
