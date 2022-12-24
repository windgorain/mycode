/* CheckOnline.js */
function CheckOnline()
{
	var bOnline = false;
	
	$.ajax(
		{ type: "Get",
			url: "/request.cgi?_do=User.Info",
			async: false,
			dataType: "json",
			success: function(oJson)
			{
                if (RQ_IsOK(oJson)) {
					bOnline = true;
				}
			}
		});
		
		return bOnline;
}
