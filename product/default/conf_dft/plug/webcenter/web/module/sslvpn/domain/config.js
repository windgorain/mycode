var g_JsNoDefs = {
  "Module":"svpn.context"
};

function MyAddInit()
{
	AdminCheckOnline();
	
	ws_service_box_init();
}

function MyModifyInit(sName)
{	
	AdminCheckOnline();
	
	$("#name").val(sName);
	var oJson = get_info(sName);
	$("#Description").val(oJson.Description);
	ws_service_box_init();
	$("#WsService").val(oJson.WsService);
}

function my_build_property()
{
	AdminCheckOnline();
	
	var sDesc = $('#Description').val();
	if (sDesc == null)
	{
		sDesc = "";
	}
	
	var sWsService = $('#WsService').val();
	if (sWsService == null)
	{
		sWsService = "";
	}
	
	var sAdmin = $('#Admin').val();
	if (sAdmin == null)
	{
		sAdmin = "";
	}
	
	var sPassword = $('#password1').val();
	if (sPassword == null)
	{
		sPassword = "";
	}

	var sRet = "&Description=" + sDesc + "&WsService=" + sWsService + "&Admin=" + sAdmin + "&AdminPassword=" + sPassword;
	
	return sRet;
}

/* 特有部分 */
function ws_service_box_init()
{
$.ajax(
	{ type: "GET",
		url: "/request.cgi?_do=ws.service.List",
		async: false,
		dataType: "json",
		success: function(oJson) {
            if (RQ_IsOK(oJson)) {
				var aGW = oJson.data;
				var x;
				for (x in aGW) {
					$("#WsService").append("<option value='" + aGW[x].Name + "'>" +aGW[x].Name + "</option>");
				}					
			}
		}
	});
}

function compare_password()
{
	if ($("#password1").val() != $("#password2").val())
	{
		return "密码不一致";
	}

	return null;
}
