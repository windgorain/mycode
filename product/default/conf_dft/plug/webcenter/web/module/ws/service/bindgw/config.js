var g_JsNoDefs = {
		"Module":"ws.service.bindgw"
	};

function MyAddInit()
{
	AdminCheckOnline();
	
	$.ajax(
		{ type: "GET",
			url: "/request.cgi?_do=ws.gateway.List",
			async: false,
			dataType: "json",
			success: function(oJson) {
                if (RQ_IsOK(oJson)) {
					var aGW = oJson.data;
					var x;
					for (x in aGW) {
						$("#Gateway").append("<option value='" + aGW[x].Name + "'>" +aGW[x].Name + "</option>");
					}					
				}
			}
		});
}

function my_build_property()
{
	AdminCheckOnline();
	
	var sService = $.query.get('Service');
	if (sService == null)
	{
		sService = "";
	}
	
	var sGateway = $('#Gateway').val();
	if (sGateway == null)
	{
		sGateway = "";
	}
	
	var sVHost = $('#VHost').val();
	if (sVHost == null)
	{
		sVHost = "";
	}
	
	var sDomain = $('#Domain').val();
	if (sDomain == null)
	{
		sDomain = "";
	}

	return "&Service=" + sService + "&Gateway=" + sGateway + "&VHost=" + sVHost + "&Domain=" + sDomain;
}

/* 特有部分 */
