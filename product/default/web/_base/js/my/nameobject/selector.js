/* 封装一个选择器，使其支持request.cgi?_do=xxx */
function selector_box_init(sEleID,sUrl,sSelected)
{
	if (sSelected == null)
	{
		sSelected = "";
	}
	
	$('#'+sEleID).empty().multiselect2side('destroy');

	$('#'+sEleID).multiselect2side({
		search: "<img src='/_base/js/jquery/multiselect2side/img/search.gif' />",
		moveOptions: false
	});
	
	$.ajax(
		{ type: "GET",
			url: sUrl,
			async: false,
			dataType: "json",
			success: function(oJson) {
				for (var index in oJson.data)
				{
					var bSelect = false;
					
					if (true == MYUTL_IsInclude(sSelected, oJson.data[index].Name, ','))
					{
						bSelect = true;
					}
					
					$('#'+sEleID).multiselect2side('addOption', {name: oJson.data[index].Name, value: oJson.data[index].Name, selected: bSelect});
				}
			}
		});
}

