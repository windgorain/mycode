/* 公开部分 */
function RQ_IsOK(oJson) {
    if (oJson.result == "Failed") {
        return false;
    }
    return true;
}

function RQ_Get(sUrl)
{
	var sRet = null;

	$.ajax(
		{ type: "GET",
			url: sUrl,
			dataType: "json",
			async: false,
			success: function(oJson) {
				if(RQ_IsOK(oJson)) {
					sRet = oJson;
				}
			}
		});

	return sRet;
}

function RQ_Post(sUrl, sData)
{
	var bResult = false;

	$.ajax(
		{ type: "POST",
			url: sUrl,
			data: sData,
			async: false,
			dataType: "json",
			success: function(oJson) {
				if(RQ_IsOK(oJson)) {
					bResult = true;
				} else {
					alert("结果: 失败\r\n原因: " + oJson.reason);
				}
			}
		});

	return bResult;
}

