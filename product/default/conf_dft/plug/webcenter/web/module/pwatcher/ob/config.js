function MyAddInit()
{
	AdminCheckOnline();
}

function MyModifyInit(sName)
{	
	AdminCheckOnline();
	
	$("#name").val(sName);
    var oJson = RQ_Get("/request.cgi?_do=cmd.list&cmd=config-view;pwatcher-view;show ob");
    var result = oJson.data.find(function(item) { return item.Name  === sName; });

	if (result.Status == "Enable")	{
		$("#Enable").prop("checked", true);
	}
}

function my_build_property()
{
	AdminCheckOnline();

	var sEnable = "";
	if ($('#Enable').prop("checked"))
	{
		sEnable = "1";
	}

	return "&Enable=" + sEnable;
}

/* 特有部分 */
function MySubmit() {
	AdminCheckOnline();
	
	var oForm = document.getElementById("MyForm");
	if (CType_CheckForm(oForm) == false) {
		return false;
	}

    var sData = "_do=cmd.run&cmd=config-view;pwatcher-view;no ob " + $("#name").val() + " enable";
    if ($('#Enable').prop("checked")) {
        sData = "_do=cmd.run&cmd=config-view;pwatcher-view;ob " + $("#name").val() + " enable";
    }

	RQ_Post("/request.cgi", sData);
	
    JS_NO_GoList();

	return true;
}

