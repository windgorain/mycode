
/* Online User 工具 */
function CheckOnline(sRedirect)
{
	var bOnline = true;
	
	$.ajax(
		{ type: "GET",
			url: "/request.cgi?_do=CheckOnline",
			async: false,
			dataType: "json",
			success: function(oJson) {
				if (oJson.online == "false")
				{
					bOnline = false;
					window.alert("请重新登录");
					window.parent.location = sRedirect;
				}
			}
		});
		
		return bOnline;
}

function AdminCheckOnline()
{
	return CheckOnline("/admin.htm");
}

function UserCheckOnline()
{
	return CheckOnline("/index.htm");
}

function ClientCheckOnline()
{
	return CheckOnline("/client/index.htm");
}

/* End */



