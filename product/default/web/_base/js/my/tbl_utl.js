
/*
 本函数用于建立一个表格Title.当然也可以用来做其他对象的标题等东西.
*/
function TBL_BuildTitle(szTitle)
{
    var szTmp = "<table cellSpacing=0 cellPadding=0 width=500 border=0>";
    var szTmp1 = "<tr><td></td><td bgColor=#929292 height=1></td><td></td></tr>";

    szTmp += szTmp1;
    szTmp += "<tr heighi=20><td bgColor=#929292 width=1></td>";
    szTmp += "<td bgColor=#e5e5e5><div align=center><b>";
    szTmp += szTitle;
    szTmp += "</b></div></td><td bgColor=#929292 width=1></td></tr>";
    szTmp += szTmp1;
    szTmp += "</table>";

    return szTmp;
}

/* 本函数用于创建一个表格 */
function TBL_Create(aArrayHeader)
{
	var obj = new Object();
	
	obj.oListHead = aArrayHeader;
	obj.aNodeArray = new Array();
	obj.bIsSupportMove = false;
	obj.ulBorderSize = 1;

	obj.AddNode = function (aArrayNode)
	{
		var oListNodeObj = new Object();

		oListNodeObj.ListNode = aArrayNode;
		
		this.aNodeArray[this.aNodeArray.length] = oListNodeObj;
	}
	
	obj.DelNode = function (szIndex)
	{
		var ulIndex = eval(szIndex);
		
		if (ulIndex >= this.aNodeArray.length)
		{
			return;
		}
		
		for (var i=ulIndex; i<this.aNodeArray.length - 1; i++)
		{
			this.aNodeArray[i] = this.aNodeArray[i+1];
		}
		
		this.aNodeArray.length --;
	}
	
	obj.SupportMove = function (bIsSupportMove)
	{
		obj.bIsSupportMove = bIsSupportMove;
	}
	
	obj.MoveUp = function (ulIndex)
	{
		if (ulIndex == 0)
		{
			return;
		}
		
		var nodeTemp = this.aNodeArray[ulIndex - 1];
		this.aNodeArray[ulIndex - 1] = this.aNodeArray[ulIndex];
		this.aNodeArray[ulIndex] = nodeTemp;
	}

	obj.MoveDown = function (ulIndex)
	{
		if (ulIndex >= this.aNodeArray.length - 1)
		{
			return;
		}
		
		var nodeTemp = this.aNodeArray[ulIndex + 1];
		this.aNodeArray[ulIndex + 1] = this.aNodeArray[ulIndex];
		this.aNodeArray[ulIndex] = nodeTemp;
	}
	
	obj.SetBorder = function (ulBorderSize)
	{
		this.ulBorderSize = ulBorderSize;
	}
	
	obj.BuildHtml = function ()
	{
		var szTmp = "<br><table border=" + this.ulBorderSize + " cellSpacing=0 cellPadding=0 bordercolor=#dddddd width=100%>";
		var ulCount = 0;
		var szColor = "";

		if (this.oListHead.length > 0)
		{
		    szTmp += "<tr>";
    		for (var i=0; i<this.oListHead.length; i++)
    		{
    			szTmp += "<td align=center bgColor=#e5e5e5><b>" + this.oListHead[i] + "</b></td>";
    			ulCount = 0;
    		}
		    szTmp += "</tr>";
		}

		for (var i=0; i<this.aNodeArray.length; i++)
		{
			if (this.aNodeArray[i].ListNode.length == 0)
			{
				continue;
			}

			if (ulCount == 0)
			{
				szColor = " bgColor=#ffffff ";
				ulCount = 1;
			}
			else
			{
				szColor = " bgColor=#ecf4fd ";
				ulCount = 0;
			}
			
			szTmp += "<tr>";

			for (var j=0; j<this.aNodeArray[i].ListNode.length; j++)
			{
				szTmp += "<td " + szColor + ">" + this.aNodeArray[i].ListNode[j] + "&nbsp</td>";
			}

			if (obj.bIsSupportMove == true)
			{
				szTmp += "<td><input type=\"button\" value=\"Up\" onClick=MoveBoxListNodeUp(" + i + ");></td>";
				szTmp += "<td><input type=\"button\" value=\"Down\" onClick=MoveBoxListNodeDown(" + i + ");></td>";
			}
			
			szTmp += "</tr>";
		}
		
		szTmp += "</table>";
		
		return szTmp;
	}
	
    return obj;
}

