
document.write('<link rel="stylesheet" type="text/css" href="/_base/js/my/toolbar/toolbar.css">');

function ToolBar_Create()
{
	var obj = new Object();
	
	obj.aLeftNode = new Array();
	obj.aRightNode = new Array();
	obj.sCurrentNodeName = "";
	
	obj.Add = function (sName, sUrl, sAlign)
	{
		if (!sAlign)
		{
			sAlign = "left";
		}
		
		var oNode = new Object();
		oNode.sName = sName.trim();
		oNode.sUrl = sUrl.trim();
		if (sAlign == "left")
		{
			this.aLeftNode[this.aLeftNode.length] = oNode;
		}
		else
		{
			this.aRightNode[this.aRightNode.length] = oNode;
		}
	}
  
	obj.AddArray = function (asNodes, sAlign)
	{
		if (!sAlign)
		{
			sAlign = "left";
		}

		for (var i=0; i<asNodes.length; i++)
		{
			var aEle = asNodes[i].splitByChar('@');
			this.Add(aEle[0], aEle[1], sAlign);
		}
	}
  
	obj.AddList = function (sNodes, sAlign)
	{
		if (!sAlign)
		{
			sAlign = "left";
		}

		var asNodes = sNodes.splitByChar(',');
		
		this.AddArray(asNodes, sAlign);
	}
  
	obj.SetCurrentName = function (sName)
	{
		this.sCurrentNodeName = sName;
	}
  
	obj.GetHtml = function ()
	{
		var sMsg = '<div class="ToolBar">';
		
		sMsg += "<ul>";
		for (var i=0; i<this.aLeftNode.length; i++)
		{
			if (this.aLeftNode[i].sName == this.sCurrentNodeName)
			{
				sMsg += '<li class="Current" > <a href=';
			}
			else
			{
				sMsg += '<li> <a href=';
			}
			sMsg += this.aLeftNode[i].sUrl;
			sMsg += '>';
			sMsg += this.aLeftNode[i].sName;
			sMsg += "</a></li>";
		}
		sMsg += "<li> &nbsp; </li> </ul>";
		
		sMsg += '<ul class="right">'
		for (var i=0; i<this.aRightNode.length; i++)
		{
			if (this.aRightNode[i].sName == this.sCurrentNodeName)
			{
				sMsg += '<li class="Current" > <a href=';
			}
			else
			{
				sMsg += '<li> <a href=';
			}
			sMsg += this.aRightNode[i].sUrl;
			sMsg += '>';
			sMsg += this.aRightNode[i].sName;
			sMsg += "</a></li>";
		}
		sMsg += "</ul>";
		
		sMsg += "</div>";
		
		return sMsg;
	}

	obj.Write = function ()
	{
		var sHtml = this.GetHtml();
		
		document.write(sHtml);
	}
  
	return obj;
}


		