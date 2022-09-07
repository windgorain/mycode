
/*
�÷�����:
1������һ��BoxList����:
	var oBoxList = CreateBoxList(new Array(x, y, z, ...));
	����...Ϊ���ű�ı�ͷ����

2����ӽڵ�:
	oBoxList.AddNode(new Array(x, y, z, ...);

3��ɾ���ڵ�:

4����ʾBoxList:
	oBoxList.ShowList(idList);
	���У�����idListΪ<p></p>�ȵ�ID

5����������Input�б�,һ���Ƿ���form�ڣ������ύBoxList�еı���
	oBoxList.CreateHideInputList(idHideInputList)
	���У�����idHideInputListΪ<p></p>�ȵ�ID

*/


function CreateBoxList(oListHead)
{
	var obj = new Object();
	
	obj.oListHead = oListHead;
	obj.aNodeArray = new Array();
	obj.bIsSupportMove = false;

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
	
	obj.DelSelectedNode = function ()
	{
		for (var i = this.aNodeArray.length; i>0; i--)
		{
			var ulIndex = i - 1;
			var oBoxListNode = document.getElementById("BoxListCheck" + ulIndex);
			
			if (oBoxListNode.checked == true)
			{
				this.DelNode(ulIndex);
			}
		}
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

	obj.ShowList = function (idList)
	{
		var szTmp = "<table border=\"1\">";

		szTmp += "<tr>";
		for (var i=0; i<this.oListHead.length; i++)
		{
			szTmp += "<td>" + this.oListHead[i] + "</td>";
		}
		szTmp += "</tr>";

		for (var i=0; i<this.aNodeArray.length; i++)
		{
			if (this.aNodeArray[i].ListNode.length == 0)
			{
				continue;
			}
			
			szTmp += "<tr>";

			szTmp += "<td><input type=\"checkbox\" name=\"BoxListCheck["
					+ this.aNodeArray[i].ListNode[0] + "]\" id=\"BoxListCheck" + i + "\"" + "></td>";
			
			for (var j=0; j<this.aNodeArray[i].ListNode.length; j++)
			{
				szTmp += "<td>" + this.aNodeArray[i].ListNode[j] + "</td>";
			}

			if (obj.bIsSupportMove == true)
			{
				szTmp += "<td><input type=\"button\" value=\"Up\" onClick=MoveBoxListNodeUp(" + i + ");></td>";
				szTmp += "<td><input type=\"button\" value=\"Down\" onClick=MoveBoxListNodeDown(" + i + ");></td>";
			}
			
			szTmp += "</tr>";
		}
		
		szTmp += "</table>";
		
		idList.innerHTML = szTmp;
	}
	
	obj.CreateHideInputList = function(idHideInputList)
	{
		var szTmp = "";
		
		for (var i=0; i<this.aNodeArray.length; i++)
		{
			for (var j=0; j<this.aNodeArray[i].ListNode.length; j++)
			{
				szTmp += "<input type=\"hidden\" name=\"BoxListCol" + j + "[" + i + "]\" value=\"" + this.aNodeArray[i].ListNode[j] + "\">";
			}
		}
		
		idHideInputList.innerHTML = szTmp;
	}
		
    return obj;
}