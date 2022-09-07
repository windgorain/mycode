/* toolbar version 2, 相比于1, 将url改为了function */

document.write('<link rel="stylesheet" type="text/css" href="/_base/js/my/toolbar/toolbar.css">');

var g_aToolBars = new Array();

function toolbar_click(index, sKey)
{
	var obj = g_aToolBars[index];
	
	obj.ClickNode(sKey);
}

function ToolBar_Create()
{
	var obj = new Object();
	
	obj.index = g_aToolBars.length;
	g_aToolBars[obj.index] = obj;
	
	obj.aNodes = new Array();
	obj.sCurrentNodeName = "";
	this.sOldNodeName = "";

	obj.Add = function (sName, pfFunc, sAlign)
	{
		if (!sAlign)
		{
			sAlign = "left";
		}

		var oNode = new Object();
		oNode.sName = sName;
		oNode.pfFunc = pfFunc;
		oNode.sAlign = sAlign;
		this.aNodes[sName] = oNode;
	}
	
	obj.ClickNode = function (sKey)
	{
		this.SetCurrentName(this.aNodes[sKey].sName);
		this.aNodes[sKey].pfFunc();
		this.Write();
	}

	obj.SetCurrentName = function (sName)
	{
		this.sOldNodeName = this.sCurrentNodeName;
		this.sCurrentNodeName = sName;
	}
	
	obj.GetCurrentName = function()
	{
		return this.sCurrentNodeName;
	}
	
	obj.GetOldName = function()
	{
		return this.sOldNodeName;
	}
	
	obj.BuildNodeHtml = function (key)
	{
		var sMsg = "";

		if (this.aNodes[key].sName == this.sCurrentNodeName)
		{
			sMsg += '<li class="Current">'
		}
		else
		{
			sMsg += '<li>'
		}
		sMsg += '<a href="#" onclick=\'toolbar_click( '+ obj.index + ',"' + key + '");return false;\'>';
		sMsg += this.aNodes[key].sName;
		sMsg += "</a></li>";
		
		return sMsg;
	}

	obj.GetHtml = function ()
	{
		var sMsg = '<div class="ToolBar">';
		
		sMsg += "<ul>";
		for (key in this.aNodes)
		{
			if (this.aNodes[key].sAlign == "left")
			{
				sMsg += this.BuildNodeHtml(key);
			}
		}
		sMsg += "<li> &nbsp; </li> </ul>";
		
		sMsg += '<ul class="right">'
		for (key in this.aNodes)
		{
			if (this.aNodes[key].sAlign == "right")
			{
				sMsg += this.BuildNodeHtml(key);
			}
		}
		sMsg += "</ul>";
		
		sMsg += "</div>";
		
		return sMsg;
	}
	
	obj.BindContainer = function (sContainerID)
	{
		this.sContainerID = sContainerID;
	}

	obj.Write = function ()
	{
		var sHtml = this.GetHtml();
		
		document.getElementById(this.sContainerID).innerHTML = sHtml;
	}
  
	return obj;
}


		