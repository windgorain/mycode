
function MyAddInit()
{
	AdminCheckOnline();
}

function MyModifyInit(sName)
{	
	AdminCheckOnline();
	
	$("#name").val(sName);
	var oJson = RQ_Get("/request.cgi?_do=Interface.Get&Name=" + sName);
	if (oJson.Shutdown == "1")	{
		$("#Shutdown").prop("checked", true);
	}

	$("#interface_info").html(oJson.Private_Info);

	oJson = RQ_Get("/request.cgi?_do=IPAddress.Get&Interface=" + sName);
	for (x in oJson.Address)	{
		$("#IPAddress").append("<option value='" + oJson.Address[x].IP + "'>" + oJson.Address[x].IP + "</option>");
	}
}

/* 特有部分 */
function AddIpAddress() {
	var sIP = $("#IP").val();
	var sMask = $("#Mask").val();
	var sIpAddress = sIP + "/" + sMask;
	$("#IPAddress").append("<option value='" + sIpAddress + "'>" + sIpAddress + "</option>");
}

function DelIpAddress() {
	var aSel = $("#IPAddress option:selected");
	for (x in aSel)	{
		aSel[x].remove();
	}
}

function MySubmit() {
	AdminCheckOnline();
	
	var bRet = true;
	
	var oForm = document.getElementById("MyForm");
	if (CType_CheckForm(oForm) == false)
	{
		return false;
	}
	
	var sData = "_do=Interface.Modify&Name=" + $("#name").val() + _my_build_if_conf();
	if (true != RQ_Post("/request.cgi", sData)) {
		bRet = false;
	}
	
	sData = "_do=IPAddress.Modify&Interface=" + $("#name").val() + _my_build_address_conf();
	if (true != RQ_Post("/request.cgi", sData)) {
		bRet = false;
	}
	
	if (bRet) {
		JS_NO_GoList();
	}	
	
	return bRet;
}

function _my_build_if_conf()
{
	var sRet = "";
	
	var sShutdown = "0";
	if ($('#Shutdown').prop("checked"))
	{
		sShutdown = "1";
	}
	
	sRet = "&Shutdown=" + sShutdown;

	return sRet;
}

function _my_build_address_conf()
{
	var sIpAddress = $("#IPAddress option").map(function(){return $(this).val();}).get().join(",");
	
	return "&IP=" + sIpAddress;
}